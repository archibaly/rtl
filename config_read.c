/* 
 * config_read.c
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

#include "debug.h"
#include "config_read.h"

#define DQUOTE		'"'
#define PAREN_OPEN	'('
#define PAREN_CLOSE	')'

#define END_LINE(c)	(c == '\n' || c == '\0')

static config_opt_t *config_table = NULL;

static char delim = '=';
static char comment = '#';

static int explode(const char *src, const char *tokens, char ***list, size_t * len);

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

config_opt_t *config_add_opt_array(const char *name, char **values,
				   const size_t size)
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

void config_set_delim(char d)
{
    delim = d;
}

int config_find_opt_value(char *name, char *value)
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
			return -1;
    }

    return 0;
}

void config_print_opt(char *name)
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
		fprintf(stdout, "VALUE [%ld] ==> %s\n", i, opt->values[i]);
}

static int parse_line(char *string)
{
    char value[255], name[255], c;
    unsigned int have_name, have_quote, have_paren;
    char **values;
    size_t i = 0, len;

    have_name = have_quote = have_paren = 0;

    while ((c = *string++) != '\0') {
		if (c == DQUOTE) {
			if (!have_name) {
				ERROR("unexpected '%c'\n", DQUOTE);
				return 0;
			}
			if (have_quote && !END_LINE(*string)) {
				ERROR("unexpected '%c' after '%c'\n", *string, DQUOTE);
				return 0;
			}
			have_quote = !have_quote;
		} else if (c == ' ') {
			/* ignore spaces outside of quotes. */
			if (have_quote)
				value[i++] = c;
		} else if (c == delim) {
			if (have_name) {
				ERROR("unexpected '%c'\n", delim);
				return 0;
			}

			have_name = 1;
			name[i] = '\0';
			i = 0;
		} else if (c == '\n') {
			break;
		} else if (c == PAREN_OPEN) {
			if (!have_name) {
				ERROR("unexpected '%c'\n", PAREN_OPEN);
				return 0;
			}
			have_paren = 1;
		} else if (c == PAREN_CLOSE) {
			if (have_paren && !END_LINE(*string)) {
				ERROR("unexpected '%c' after '%c'\n", *string, PAREN_CLOSE);
				return 0;
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

    return -1;
}

int config_load(const char *filename)
{
    FILE *fp;
    char line[255];

    if ((fp = fopen(filename, "r")) == NULL)
		return 0;

    while (fgets(line, sizeof(line), fp) != NULL) {
		/* ignore lines that start with a comment character */
		if (*line == comment)
			continue;

		if (!parse_line(line)) {
			fclose(fp);
			return 0;
		}
    }

    fclose(fp);
    return -1;
}

void config_free_opt(config_opt_t * opt)
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

static int explode(const char *src, const char *tokens, char ***list, size_t * len)
{
    char *str, *copy, **_list = NULL;

    if (src == NULL || list == NULL || len == NULL)
		return 0;

    *list = NULL;
    *len = 0;

    copy = strdup(src);

    str = strtok(copy, tokens);

    if (str == NULL) {
		free(copy);
		return 0;
    }

    if ((_list = malloc(sizeof(*_list))) == NULL)
		exit(EXIT_FAILURE);

    _list[*len] = strdup(str);

    (*len)++;

    while ((str = strtok(NULL, tokens)) != NULL) {
		if ((_list = realloc(_list, (*len + 1) * sizeof(*_list))) == NULL)
			exit(EXIT_FAILURE);

		_list[*len] = strdup(str);

		(*len)++;
    }

    *list = _list;
    free(copy);

    return -1;
}
