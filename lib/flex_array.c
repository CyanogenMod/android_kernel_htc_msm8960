/*
 * Flexible array managed in PAGE_SIZE parts
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Copyright IBM Corporation, 2009
 *
 * Author: Dave Hansen <dave@linux.vnet.ibm.com>
 */

#include <linux/flex_array.h>
#include <linux/slab.h>
#include <linux/stddef.h>
#include <linux/export.h>
#include <linux/reciprocal_div.h>

struct flex_array_part {
	char elements[FLEX_ARRAY_PART_SIZE];
};

static inline int elements_fit_in_base(struct flex_array *fa)
{
	int data_size = fa->element_size * fa->total_nr_elements;
	if (data_size <= FLEX_ARRAY_BASE_BYTES_LEFT)
		return 1;
	return 0;
}

struct flex_array *flex_array_alloc(int element_size, unsigned int total,
					gfp_t flags)
{
	struct flex_array *ret;
	int elems_per_part = 0;
	int reciprocal_elems = 0;
	int max_size = 0;

	if (element_size) {
		elems_per_part = FLEX_ARRAY_ELEMENTS_PER_PART(element_size);
		reciprocal_elems = reciprocal_value(elems_per_part);
		max_size = FLEX_ARRAY_NR_BASE_PTRS * elems_per_part;
	}

	
	if (total > max_size)
		return NULL;
	ret = kzalloc(sizeof(struct flex_array), flags);
	if (!ret)
		return NULL;
	ret->element_size = element_size;
	ret->total_nr_elements = total;
	ret->elems_per_part = elems_per_part;
	ret->reciprocal_elems = reciprocal_elems;
	if (elements_fit_in_base(ret) && !(flags & __GFP_ZERO))
		memset(&ret->parts[0], FLEX_ARRAY_FREE,
						FLEX_ARRAY_BASE_BYTES_LEFT);
	return ret;
}
EXPORT_SYMBOL(flex_array_alloc);

static int fa_element_to_part_nr(struct flex_array *fa,
					unsigned int element_nr)
{
	return reciprocal_divide(element_nr, fa->reciprocal_elems);
}

void flex_array_free_parts(struct flex_array *fa)
{
	int part_nr;

	if (elements_fit_in_base(fa))
		return;
	for (part_nr = 0; part_nr < FLEX_ARRAY_NR_BASE_PTRS; part_nr++)
		kfree(fa->parts[part_nr]);
}
EXPORT_SYMBOL(flex_array_free_parts);

void flex_array_free(struct flex_array *fa)
{
	flex_array_free_parts(fa);
	kfree(fa);
}
EXPORT_SYMBOL(flex_array_free);

static unsigned int index_inside_part(struct flex_array *fa,
					unsigned int element_nr,
					unsigned int part_nr)
{
	unsigned int part_offset;

	part_offset = element_nr - part_nr * fa->elems_per_part;
	return part_offset * fa->element_size;
}

static struct flex_array_part *
__fa_get_part(struct flex_array *fa, int part_nr, gfp_t flags)
{
	struct flex_array_part *part = fa->parts[part_nr];
	if (!part) {
		part = kmalloc(sizeof(struct flex_array_part), flags);
		if (!part)
			return NULL;
		if (!(flags & __GFP_ZERO))
			memset(part, FLEX_ARRAY_FREE,
				sizeof(struct flex_array_part));
		fa->parts[part_nr] = part;
	}
	return part;
}

int flex_array_put(struct flex_array *fa, unsigned int element_nr, void *src,
			gfp_t flags)
{
	int part_nr = 0;
	struct flex_array_part *part;
	void *dst;

	if (element_nr >= fa->total_nr_elements)
		return -ENOSPC;
	if (!fa->element_size)
		return 0;
	if (elements_fit_in_base(fa))
		part = (struct flex_array_part *)&fa->parts[0];
	else {
		part_nr = fa_element_to_part_nr(fa, element_nr);
		part = __fa_get_part(fa, part_nr, flags);
		if (!part)
			return -ENOMEM;
	}
	dst = &part->elements[index_inside_part(fa, element_nr, part_nr)];
	memcpy(dst, src, fa->element_size);
	return 0;
}
EXPORT_SYMBOL(flex_array_put);

int flex_array_clear(struct flex_array *fa, unsigned int element_nr)
{
	int part_nr = 0;
	struct flex_array_part *part;
	void *dst;

	if (element_nr >= fa->total_nr_elements)
		return -ENOSPC;
	if (!fa->element_size)
		return 0;
	if (elements_fit_in_base(fa))
		part = (struct flex_array_part *)&fa->parts[0];
	else {
		part_nr = fa_element_to_part_nr(fa, element_nr);
		part = fa->parts[part_nr];
		if (!part)
			return -EINVAL;
	}
	dst = &part->elements[index_inside_part(fa, element_nr, part_nr)];
	memset(dst, FLEX_ARRAY_FREE, fa->element_size);
	return 0;
}
EXPORT_SYMBOL(flex_array_clear);

int flex_array_prealloc(struct flex_array *fa, unsigned int start,
			unsigned int nr_elements, gfp_t flags)
{
	int start_part;
	int end_part;
	int part_nr;
	unsigned int end;
	struct flex_array_part *part;

	if (!start && !nr_elements)
		return 0;
	if (start >= fa->total_nr_elements)
		return -ENOSPC;
	if (!nr_elements)
		return 0;

	end = start + nr_elements - 1;

	if (end >= fa->total_nr_elements)
		return -ENOSPC;
	if (!fa->element_size)
		return 0;
	if (elements_fit_in_base(fa))
		return 0;
	start_part = fa_element_to_part_nr(fa, start);
	end_part = fa_element_to_part_nr(fa, end);
	for (part_nr = start_part; part_nr <= end_part; part_nr++) {
		part = __fa_get_part(fa, part_nr, flags);
		if (!part)
			return -ENOMEM;
	}
	return 0;
}
EXPORT_SYMBOL(flex_array_prealloc);

void *flex_array_get(struct flex_array *fa, unsigned int element_nr)
{
	int part_nr = 0;
	struct flex_array_part *part;

	if (!fa->element_size)
		return NULL;
	if (element_nr >= fa->total_nr_elements)
		return NULL;
	if (elements_fit_in_base(fa))
		part = (struct flex_array_part *)&fa->parts[0];
	else {
		part_nr = fa_element_to_part_nr(fa, element_nr);
		part = fa->parts[part_nr];
		if (!part)
			return NULL;
	}
	return &part->elements[index_inside_part(fa, element_nr, part_nr)];
}
EXPORT_SYMBOL(flex_array_get);

void *flex_array_get_ptr(struct flex_array *fa, unsigned int element_nr)
{
	void **tmp;

	tmp = flex_array_get(fa, element_nr);
	if (!tmp)
		return NULL;

	return *tmp;
}
EXPORT_SYMBOL(flex_array_get_ptr);

static int part_is_free(struct flex_array_part *part)
{
	int i;

	for (i = 0; i < sizeof(struct flex_array_part); i++)
		if (part->elements[i] != FLEX_ARRAY_FREE)
			return 0;
	return 1;
}

int flex_array_shrink(struct flex_array *fa)
{
	struct flex_array_part *part;
	int part_nr;
	int ret = 0;

	if (!fa->total_nr_elements || !fa->element_size)
		return 0;
	if (elements_fit_in_base(fa))
		return ret;
	for (part_nr = 0; part_nr < FLEX_ARRAY_NR_BASE_PTRS; part_nr++) {
		part = fa->parts[part_nr];
		if (!part)
			continue;
		if (part_is_free(part)) {
			fa->parts[part_nr] = NULL;
			kfree(part);
			ret++;
		}
	}
	return ret;
}
EXPORT_SYMBOL(flex_array_shrink);
