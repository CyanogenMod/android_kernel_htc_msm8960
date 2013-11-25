/*
 * Copyright (C) 2002-2005 Roman Zippel <zippel@linux-m68k.org>
 * Copyright (C) 2002-2005 Sam Ravnborg <sam@ravnborg.org>
 *
 * Released under the terms of the GNU GPL v2.0.
 */

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "lkc.h"

struct file *file_lookup(const char *name)
{
	struct file *file;
	const char *file_name = sym_expand_string_value(name);

	for (file = file_list; file; file = file->next) {
		if (!strcmp(name, file->name)) {
			free((void *)file_name);
			return file;
		}
	}

	file = malloc(sizeof(*file));
	memset(file, 0, sizeof(*file));
	file->name = file_name;
	file->next = file_list;
	file_list = file;
	return file;
}

int file_write_dep(const char *name)
{
	struct symbol *sym, *env_sym;
	struct expr *e;
	struct file *file;
	FILE *out;

	if (!name)
		name = ".kconfig.d";
	out = fopen("..config.tmp", "w");
	if (!out)
		return 1;
	fprintf(out, "deps_config := \\\n");
	for (file = file_list; file; file = file->next) {
		if (file->next)
			fprintf(out, "\t%s \\\n", file->name);
		else
			fprintf(out, "\t%s\n", file->name);
	}
	fprintf(out, "\n%s: \\\n"
		     "\t$(deps_config)\n\n", conf_get_autoconfig_name());

	expr_list_for_each_sym(sym_env_list, e, sym) {
		struct property *prop;
		const char *value;

		prop = sym_get_env_prop(sym);
		env_sym = prop_get_symbol(prop);
		if (!env_sym)
			continue;
		value = getenv(env_sym->name);
		if (!value)
			value = "";
		fprintf(out, "ifneq \"$(%s)\" \"%s\"\n", env_sym->name, value);
		fprintf(out, "%s: FORCE\n", conf_get_autoconfig_name());
		fprintf(out, "endif\n");
	}

	fprintf(out, "\n$(deps_config): ;\n");
	fclose(out);
	rename("..config.tmp", name);
	return 0;
}


struct gstr str_new(void)
{
	struct gstr gs;
	gs.s = malloc(sizeof(char) * 64);
	gs.len = 64;
	gs.max_width = 0;
	strcpy(gs.s, "\0");
	return gs;
}

struct gstr str_assign(const char *s)
{
	struct gstr gs;
	gs.s = strdup(s);
	gs.len = strlen(s) + 1;
	gs.max_width = 0;
	return gs;
}

void str_free(struct gstr *gs)
{
	if (gs->s)
		free(gs->s);
	gs->s = NULL;
	gs->len = 0;
}

void str_append(struct gstr *gs, const char *s)
{
	size_t l;
	if (s) {
		l = strlen(gs->s) + strlen(s) + 1;
		if (l > gs->len) {
			gs->s   = realloc(gs->s, l);
			gs->len = l;
		}
		strcat(gs->s, s);
	}
}

void str_printf(struct gstr *gs, const char *fmt, ...)
{
	va_list ap;
	char s[10000]; 
	va_start(ap, fmt);
	vsnprintf(s, sizeof(s), fmt, ap);
	str_append(gs, s);
	va_end(ap);
}

const char *str_get(struct gstr *gs)
{
	return gs->s;
}

