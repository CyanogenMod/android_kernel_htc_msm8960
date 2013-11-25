/*
 * Updated: Trusted Computer Solutions, Inc. <dgoeddel@trustedcs.com>
 *
 *	Support for enhanced MLS infrastructure.
 *
 * Copyright (C) 2004-2006 Trusted Computer Solutions, Inc.
 */
/*
 * Updated: Hewlett-Packard <paul@paul-moore.com>
 *
 *      Added support to import/export the MLS label from NetLabel
 *
 * (c) Copyright Hewlett-Packard Development Company, L.P., 2006
 */

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <net/netlabel.h>
#include "sidtab.h"
#include "mls.h"
#include "policydb.h"
#include "services.h"

int mls_compute_context_len(struct context *context)
{
	int i, l, len, head, prev;
	char *nm;
	struct ebitmap *e;
	struct ebitmap_node *node;

	if (!policydb.mls_enabled)
		return 0;

	len = 1; 
	for (l = 0; l < 2; l++) {
		int index_sens = context->range.level[l].sens;
		len += strlen(sym_name(&policydb, SYM_LEVELS, index_sens - 1));

		
		head = -2;
		prev = -2;
		e = &context->range.level[l].cat;
		ebitmap_for_each_positive_bit(e, node, i) {
			if (i - prev > 1) {
				
				if (head != prev) {
					nm = sym_name(&policydb, SYM_CATS, prev);
					len += strlen(nm) + 1;
				}
				nm = sym_name(&policydb, SYM_CATS, i);
				len += strlen(nm) + 1;
				head = i;
			}
			prev = i;
		}
		if (prev != head) {
			nm = sym_name(&policydb, SYM_CATS, prev);
			len += strlen(nm) + 1;
		}
		if (l == 0) {
			if (mls_level_eq(&context->range.level[0],
					 &context->range.level[1]))
				break;
			else
				len++;
		}
	}

	return len;
}

void mls_sid_to_context(struct context *context,
			char **scontext)
{
	char *scontextp, *nm;
	int i, l, head, prev;
	struct ebitmap *e;
	struct ebitmap_node *node;

	if (!policydb.mls_enabled)
		return;

	scontextp = *scontext;

	*scontextp = ':';
	scontextp++;

	for (l = 0; l < 2; l++) {
		strcpy(scontextp, sym_name(&policydb, SYM_LEVELS,
					   context->range.level[l].sens - 1));
		scontextp += strlen(scontextp);

		
		head = -2;
		prev = -2;
		e = &context->range.level[l].cat;
		ebitmap_for_each_positive_bit(e, node, i) {
			if (i - prev > 1) {
				
				if (prev != head) {
					if (prev - head > 1)
						*scontextp++ = '.';
					else
						*scontextp++ = ',';
					nm = sym_name(&policydb, SYM_CATS, prev);
					strcpy(scontextp, nm);
					scontextp += strlen(nm);
				}
				if (prev < 0)
					*scontextp++ = ':';
				else
					*scontextp++ = ',';
				nm = sym_name(&policydb, SYM_CATS, i);
				strcpy(scontextp, nm);
				scontextp += strlen(nm);
				head = i;
			}
			prev = i;
		}

		if (prev != head) {
			if (prev - head > 1)
				*scontextp++ = '.';
			else
				*scontextp++ = ',';
			nm = sym_name(&policydb, SYM_CATS, prev);
			strcpy(scontextp, nm);
			scontextp += strlen(nm);
		}

		if (l == 0) {
			if (mls_level_eq(&context->range.level[0],
					 &context->range.level[1]))
				break;
			else
				*scontextp++ = '-';
		}
	}

	*scontext = scontextp;
	return;
}

int mls_level_isvalid(struct policydb *p, struct mls_level *l)
{
	struct level_datum *levdatum;
	struct ebitmap_node *node;
	int i;

	if (!l->sens || l->sens > p->p_levels.nprim)
		return 0;
	levdatum = hashtab_search(p->p_levels.table,
				  sym_name(p, SYM_LEVELS, l->sens - 1));
	if (!levdatum)
		return 0;

	ebitmap_for_each_positive_bit(&l->cat, node, i) {
		if (i > p->p_cats.nprim)
			return 0;
		if (!ebitmap_get_bit(&levdatum->level->cat, i)) {
			return 0;
		}
	}

	return 1;
}

int mls_range_isvalid(struct policydb *p, struct mls_range *r)
{
	return (mls_level_isvalid(p, &r->level[0]) &&
		mls_level_isvalid(p, &r->level[1]) &&
		mls_level_dom(&r->level[1], &r->level[0]));
}

int mls_context_isvalid(struct policydb *p, struct context *c)
{
	struct user_datum *usrdatum;

	if (!p->mls_enabled)
		return 1;

	if (!mls_range_isvalid(p, &c->range))
		return 0;

	if (c->role == OBJECT_R_VAL)
		return 1;

	if (!c->user || c->user > p->p_users.nprim)
		return 0;
	usrdatum = p->user_val_to_struct[c->user - 1];
	if (!mls_range_contains(usrdatum->range, c->range))
		return 0; 

	return 1;
}

int mls_context_to_sid(struct policydb *pol,
		       char oldc,
		       char **scontext,
		       struct context *context,
		       struct sidtab *s,
		       u32 def_sid)
{

	char delim;
	char *scontextp, *p, *rngptr;
	struct level_datum *levdatum;
	struct cat_datum *catdatum, *rngdatum;
	int l, rc = -EINVAL;

	if (!pol->mls_enabled) {
		if (def_sid != SECSID_NULL && oldc)
			*scontext += strlen(*scontext) + 1;
		return 0;
	}

	if (!oldc) {
		struct context *defcon;

		if (def_sid == SECSID_NULL)
			goto out;

		defcon = sidtab_search(s, def_sid);
		if (!defcon)
			goto out;

		rc = mls_context_cpy(context, defcon);
		goto out;
	}

	
	scontextp = p = *scontext;
	while (*p && *p != ':' && *p != '-')
		p++;

	delim = *p;
	if (delim != '\0')
		*p++ = '\0';

	for (l = 0; l < 2; l++) {
		levdatum = hashtab_search(pol->p_levels.table, scontextp);
		if (!levdatum) {
			rc = -EINVAL;
			goto out;
		}

		context->range.level[l].sens = levdatum->level->sens;

		if (delim == ':') {
			
			while (1) {
				scontextp = p;
				while (*p && *p != ',' && *p != '-')
					p++;
				delim = *p;
				if (delim != '\0')
					*p++ = '\0';

				
				rngptr = strchr(scontextp, '.');
				if (rngptr != NULL) {
					
					*rngptr++ = '\0';
				}

				catdatum = hashtab_search(pol->p_cats.table,
							  scontextp);
				if (!catdatum) {
					rc = -EINVAL;
					goto out;
				}

				rc = ebitmap_set_bit(&context->range.level[l].cat,
						     catdatum->value - 1, 1);
				if (rc)
					goto out;

				
				if (rngptr) {
					int i;

					rngdatum = hashtab_search(pol->p_cats.table, rngptr);
					if (!rngdatum) {
						rc = -EINVAL;
						goto out;
					}

					if (catdatum->value >= rngdatum->value) {
						rc = -EINVAL;
						goto out;
					}

					for (i = catdatum->value; i < rngdatum->value; i++) {
						rc = ebitmap_set_bit(&context->range.level[l].cat, i, 1);
						if (rc)
							goto out;
					}
				}

				if (delim != ',')
					break;
			}
		}
		if (delim == '-') {
			
			scontextp = p;
			while (*p && *p != ':')
				p++;

			delim = *p;
			if (delim != '\0')
				*p++ = '\0';
		} else
			break;
	}

	if (l == 0) {
		context->range.level[1].sens = context->range.level[0].sens;
		rc = ebitmap_cpy(&context->range.level[1].cat,
				 &context->range.level[0].cat);
		if (rc)
			goto out;
	}
	*scontext = ++p;
	rc = 0;
out:
	return rc;
}

int mls_from_string(char *str, struct context *context, gfp_t gfp_mask)
{
	char *tmpstr, *freestr;
	int rc;

	if (!policydb.mls_enabled)
		return -EINVAL;

	tmpstr = freestr = kstrdup(str, gfp_mask);
	if (!tmpstr) {
		rc = -ENOMEM;
	} else {
		rc = mls_context_to_sid(&policydb, ':', &tmpstr, context,
					NULL, SECSID_NULL);
		kfree(freestr);
	}

	return rc;
}

int mls_range_set(struct context *context,
				struct mls_range *range)
{
	int l, rc = 0;

	
	for (l = 0; l < 2; l++) {
		context->range.level[l].sens = range->level[l].sens;
		rc = ebitmap_cpy(&context->range.level[l].cat,
				 &range->level[l].cat);
		if (rc)
			break;
	}

	return rc;
}

int mls_setup_user_range(struct context *fromcon, struct user_datum *user,
			 struct context *usercon)
{
	if (policydb.mls_enabled) {
		struct mls_level *fromcon_sen = &(fromcon->range.level[0]);
		struct mls_level *fromcon_clr = &(fromcon->range.level[1]);
		struct mls_level *user_low = &(user->range.level[0]);
		struct mls_level *user_clr = &(user->range.level[1]);
		struct mls_level *user_def = &(user->dfltlevel);
		struct mls_level *usercon_sen = &(usercon->range.level[0]);
		struct mls_level *usercon_clr = &(usercon->range.level[1]);

		
		if (mls_level_between(user_def, fromcon_sen, fromcon_clr))
			*usercon_sen = *user_def;
		else if (mls_level_between(fromcon_sen, user_def, user_clr))
			*usercon_sen = *fromcon_sen;
		else if (mls_level_between(fromcon_clr, user_low, user_def))
			*usercon_sen = *user_low;
		else
			return -EINVAL;

		if (mls_level_dom(user_clr, fromcon_clr))
			*usercon_clr = *fromcon_clr;
		else if (mls_level_dom(fromcon_clr, user_clr))
			*usercon_clr = *user_clr;
		else
			return -EINVAL;
	}

	return 0;
}

int mls_convert_context(struct policydb *oldp,
			struct policydb *newp,
			struct context *c)
{
	struct level_datum *levdatum;
	struct cat_datum *catdatum;
	struct ebitmap bitmap;
	struct ebitmap_node *node;
	int l, i;

	if (!policydb.mls_enabled)
		return 0;

	for (l = 0; l < 2; l++) {
		levdatum = hashtab_search(newp->p_levels.table,
					  sym_name(oldp, SYM_LEVELS,
						   c->range.level[l].sens - 1));

		if (!levdatum)
			return -EINVAL;
		c->range.level[l].sens = levdatum->level->sens;

		ebitmap_init(&bitmap);
		ebitmap_for_each_positive_bit(&c->range.level[l].cat, node, i) {
			int rc;

			catdatum = hashtab_search(newp->p_cats.table,
						  sym_name(oldp, SYM_CATS, i));
			if (!catdatum)
				return -EINVAL;
			rc = ebitmap_set_bit(&bitmap, catdatum->value - 1, 1);
			if (rc)
				return rc;
		}
		ebitmap_destroy(&c->range.level[l].cat);
		c->range.level[l].cat = bitmap;
	}

	return 0;
}

int mls_compute_sid(struct context *scontext,
		    struct context *tcontext,
		    u16 tclass,
		    u32 specified,
		    struct context *newcontext,
		    bool sock)
{
	struct range_trans rtr;
	struct mls_range *r;

	if (!policydb.mls_enabled)
		return 0;

	switch (specified) {
	case AVTAB_TRANSITION:
		
		rtr.source_type = scontext->type;
		rtr.target_type = tcontext->type;
		rtr.target_class = tclass;
		r = hashtab_search(policydb.range_tr, &rtr);
		if (r)
			return mls_range_set(newcontext, r);
		
	case AVTAB_CHANGE:
		if ((tclass == policydb.process_class) || (sock == true))
			
			return mls_context_cpy(newcontext, scontext);
		else
			
			return mls_context_cpy_low(newcontext, scontext);
	case AVTAB_MEMBER:
		
		return mls_context_cpy_low(newcontext, scontext);

	
	}
	return -EINVAL;
}

#ifdef CONFIG_NETLABEL
void mls_export_netlbl_lvl(struct context *context,
			   struct netlbl_lsm_secattr *secattr)
{
	if (!policydb.mls_enabled)
		return;

	secattr->attr.mls.lvl = context->range.level[0].sens - 1;
	secattr->flags |= NETLBL_SECATTR_MLS_LVL;
}

void mls_import_netlbl_lvl(struct context *context,
			   struct netlbl_lsm_secattr *secattr)
{
	if (!policydb.mls_enabled)
		return;

	context->range.level[0].sens = secattr->attr.mls.lvl + 1;
	context->range.level[1].sens = context->range.level[0].sens;
}

int mls_export_netlbl_cat(struct context *context,
			  struct netlbl_lsm_secattr *secattr)
{
	int rc;

	if (!policydb.mls_enabled)
		return 0;

	rc = ebitmap_netlbl_export(&context->range.level[0].cat,
				   &secattr->attr.mls.cat);
	if (rc == 0 && secattr->attr.mls.cat != NULL)
		secattr->flags |= NETLBL_SECATTR_MLS_CAT;

	return rc;
}

int mls_import_netlbl_cat(struct context *context,
			  struct netlbl_lsm_secattr *secattr)
{
	int rc;

	if (!policydb.mls_enabled)
		return 0;

	rc = ebitmap_netlbl_import(&context->range.level[0].cat,
				   secattr->attr.mls.cat);
	if (rc != 0)
		goto import_netlbl_cat_failure;

	rc = ebitmap_cpy(&context->range.level[1].cat,
			 &context->range.level[0].cat);
	if (rc != 0)
		goto import_netlbl_cat_failure;

	return 0;

import_netlbl_cat_failure:
	ebitmap_destroy(&context->range.level[0].cat);
	ebitmap_destroy(&context->range.level[1].cat);
	return rc;
}
#endif 
