/* 
 * config.c
 *
 * Copyright (c) 2012 "config" Niels Vanden Eynde 
 * 
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.*
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

#include "debug.h"
#include "config.h"

#define DQUOTE		'"'
#define PAREN_OPEN	'('
#define PAREN_CLOSE	')'

#define END_LINE(c)	(c == '\n' || c == '\0')

static config_opt_t *config_table = NULL;

static char delim = '=';
static char comment = '#';

static int explode(const char *src, const char *tokens, char ***list, size_t *len);

config_opt_t *new_config_opt(const char *name, const char *value)
{
	config_opt_t *opt;

	if ((opt = malloc(sizeof(config_opt_t))) == NULL)
		exit(EXIT_FAILURE);

	opt->name = strdup(name);
	opt->value = value == NULL ? NULL : strdup(value);

	opt->is_array = 0;
	opt->size = 1;

	return opt;
}

config_opt_t *config_add_opt(const char *name, const char *value)
{
	config_opt_t *opt;

	HASH_FIND_STR(config_table, name, opt);
	if (opt == NULL) {
		opt = new_config_opt(name, value);
		HASH_ADD_STR(config_table, name, opt);
	}

	return opt;
}

config_opt_t *config_add_opt_array(const char *name, char **values, size_t size)
{
	config_opt_t *opt;

	HASH_FIND_STR(config_table, name, opt);
	if (opt == NULL) {
		opt = new_config_opt(name, NULL);
		opt->values = values;
		opt->is_array = 1;
		opt->size = size;
		HASH_ADD_STR(config_table, name, opt);
	}

	return opt;
}

config_opt_t *config_get_opt(const char *name)
{
	config_opt_t *opt;

	HASH_FIND_STR(config_table, name, opt);

	return opt;
}

char *config_get_value(const char *name)
{
	char *value;
	config_opt_t *opt;

	opt = config_get_opt(name);

	if (opt)
		value = opt->value;
	else
		value = NULL;

	return value;
}

void config_set_value(const char *name, const char *value)
{
	config_opt_t *opt;
	opt = config_get_opt(name);
	if (opt) {
		free(opt->value);
		opt->value = value == NULL ? NULL : strdup(value);
	} else {
		config_add_opt(name, value);
	}

}

/*
 * example:
 *     value = father,mother,sisters
 */
void config_set_value_array(const char *name, const char *value)
{
	size_t i;
	size_t len;
	char **values;

	config_opt_t *opt;
	opt = config_get_opt(name);
	explode(value, ",", &values, &len);
	if (opt) {
		for (i = 0; i < opt->size; i++)
			free(opt->values[i]);
		free(opt->values);
		opt->values = values;
		opt->size = len;
	} else {
		config_add_opt_array(name, values, len);
	}
}

void config_set_delim(char d)
{
	delim = d;
}

/*
 * @return: 1 - found value
 *          0 - not found
 */
int config_find_opt_value(const char *name, const char *value)
{
	size_t i;
	config_opt_t *opt;

	opt = config_get_opt(name);
	if (opt == NULL)
		return 0;
	if (!opt->is_array)
		return 0;
	for (i = 0; i < opt->size; i++) {
		if (strcmp(opt->values[i], value) == 0)
			return 1;
	}

	return 0;
}

void config_print_opt(const char *name)
{
	size_t i;
	config_opt_t *opt;

	opt = config_get_opt(name);

	if (opt == NULL)
		puts("NULL ==> NULL");

	if (!opt->is_array) {
		fprintf(stdout, "NAME ==> %s\n", opt->name);
		fprintf(stdout, "VALUE ==> %s\n", opt->value);
		return;
	}

	fprintf(stdout, "NAME ==> %s\n", opt->name);

	for (i = 0; i < opt->size; i++)
		fprintf(stdout, "VALUE [%d] ==> %s\n", i, opt->values[i]);
}

static int parse_line(char *string)
{
	char value[256], name[256], c;
	unsigned int have_name, have_quote, have_paren;
	char **values;
	size_t i = 0, len;

	have_name = have_quote = have_paren = 0;

	while ((c = *string++) != '\0') {
		if (c == DQUOTE) {
			if (!have_name) {
				debug("unexpected '%c'", DQUOTE);
				return -1;
			}
			if (have_quote && !END_LINE(*string) && !have_paren) {
				debug("unexpected '%c' after '%c'", *string, DQUOTE);
				return -1;
			}
			if (have_quote && have_paren && *string != ',' && *string != PAREN_CLOSE) {
				debug("unexpected '%c' after '%c'", *string, DQUOTE);
				return -1;
			}
			have_quote = !have_quote;
		} else if (c == ' ') {
			/* ignore spaces outside of quotes. */
			if (have_quote)
				value[i++] = c;
		} else if (c == delim) {
			if (have_name) {
				debug("unexpected '%c'", delim);
				return -1;
			}

			have_name = 1;
			name[i] = '\0';
			i = 0;
		} else if (c == '\n') {
			break;
		} else if (c == PAREN_OPEN) {
			if (!have_name) {
				debug("unexpected '%c'", PAREN_OPEN);
				return -1;
			}
			have_paren = 1;
		} else if (c == PAREN_CLOSE) {
			if (have_paren && !END_LINE(*string)) {
				debug("unexpected '%c' after '%c'", *string, PAREN_CLOSE);
				return -1;
			}
		} else {
			if (have_name)
				value[i++] = c;
			else
				name[i++] = c;
		}
	}

	value[i] = '\0';

	if (!have_name)
		return -1;

	if (have_paren) {
		explode(value, ",", &values, &len);
		config_add_opt_array(name, values, len);
	} else {
		config_add_opt(name, value);
	}

	return 0;
}

/*
 * load config file
 * @return: 0 - success;
 *         -1 - failed
 */
int config_load(const char *filename)
{
	FILE *fp;
	char line[1024];

	if ((fp = fopen(filename, "r")) == NULL)
		return -1;

	while (fgets(line, sizeof(line), fp) != NULL) {
		/* ignore lines that start with a comment character */
		if (*line == comment)
			continue;

		if (parse_line(line) < 0) {
			fclose(fp);
			return -1;
		}
	}

	fclose(fp);
	return 0;
}

static int has_space(const char *str)
{
	int i;
	for (i = 0; str[i] != '\0'; i++) {
		if (isspace(str[i]))
			return 1;
	}
	return 0;
}

int config_save(const char *filename)
{
	FILE *fp;
	char line[1024];
	size_t i;
	config_opt_t *opt, *tmp;

	if ((fp = fopen(filename, "w")) == NULL)
		return -1;

	HASH_ITER(hh, config_table, opt, tmp) {
		if (opt->is_array) {
			sprintf(line, "%s %c %c", opt->name, delim, PAREN_OPEN);
			for (i = 0; i < opt->size; i++) {
				if (has_space(opt->values[i]))
					sprintf(line + strlen(line), "\"%s\",", opt->values[i]);
				else
					sprintf(line + strlen(line), "%s,", opt->values[i]);
			}
			sprintf(line + strlen(line) - 1, "%c\n", PAREN_CLOSE);
		} else {
			if (has_space(opt->value))
				sprintf(line, "%s %c \"%s\"\n", opt->name, delim, opt->value);
			else
				sprintf(line, "%s %c %s\n", opt->name, delim, opt->value);
		}
		fputs(line, fp);
	}
	fclose(fp);
	return 0;
}

void config_free_opt(config_opt_t *opt)
{
	size_t i;

	if (opt->is_array) {
		for (i = 0; i < opt->size; i++)
			free(opt->values[i]);
		free(opt->values);
	} else {
		free(opt->value);
	}

	free(opt->name);
	free(opt);
}

void config_free(void)
{
	config_opt_t *opt, *tmp;
	HASH_ITER(hh, config_table, opt, tmp) {
		config_free_opt(opt);
	}
}

static int explode(const char *src, const char *tokens, char ***list, size_t *len)
{
	char *str, *copy, **_list = NULL;

	if (src == NULL || list == NULL || len == NULL)
		return -1;

	*list = NULL;
	*len = 0;

	copy = strdup(src);

	while ((str = strsep(&copy, tokens)) != NULL) {
		if ((_list = realloc(_list, (*len + 1) * sizeof(*_list))) == NULL)
			exit(EXIT_FAILURE);
		_list[*len] = strdup(str);
		(*len)++;
	}

	*list = _list;
	free(copy);

	return 0;
}
