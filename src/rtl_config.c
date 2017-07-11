#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

#include "rtl_hash.h"
#include "rtl_config.h"

#define END_LINE(c)			(c == '\n' || c == '\0')

struct config_item {
	char *key;
	char *value;
	rtl_hash_handle_t hh;
};

static char delim = '=';
static char comment = '#';
static struct config_item *items;

int rtl_config_add(const char *key, const char *value)
{
	struct config_item *c;

	RTL_HASH_FIND_STR(items, key, c);
	if (!c) {
		c = malloc(sizeof(struct config_item));
		if (!c)
			return -1;
		c->key = strdup(key);
		if (!c->key) {
			free(c);
			return -1;
		}
		c->value = strdup(value);
		if (!c->value) {
			free(c->key);
			free(c);
			return -1;
		}
		RTL_HASH_ADD_STR(items, key, c);
	}

	return 0;
}

void rtl_config_del(const char *key)
{
	struct config_item *c;

	RTL_HASH_FIND_STR(items, key, c);
	if (!c)
		return;

	RTL_HASH_DEL(items, c);
	free(c->key);
	free(c->value);
	free(c);
}

char *rtl_config_get_value(const char *key)
{
	struct config_item *c;

	RTL_HASH_FIND_STR(items, key, c);
	if (!c)
		return NULL;
	return c->value;
}

void rtl_config_set_delim(char d)
{
	delim = d;
}

void rtl_config_set_comment(char c)
{
	comment = c;
}

static int parse_line(char *string)
{
	char key[512], value[512], c;
	int have_key, have_quote;
	int i = 0;

	have_key = have_quote = 0;

	while ((c = *string++) != '\0') {
		if (c == '"') {
			if (!have_key) {
				fprintf(stderr, "unexpected '%c'", '"');
				return -1;
			}
			if (have_quote && !END_LINE(*string)) {
				fprintf(stderr, "unexpected '%c' after '%c'", *string, '"');
				return -1;
			}
			have_quote = !have_quote;
		} else if (c == ' ') {
			/* ignore spaces outside of quotes. */
			if (have_quote)
				value[i++] = c;
		} else if (c == delim) {
			if (have_key) {
				fprintf(stderr, "unexpected '%c'", delim);
				return -1;
			}
			have_key = 1;
			key[i] = '\0';
			i = 0;
		} else if (c == '\n') {
			break;
		} else {
			if (have_key)
				value[i++] = c;
			else
				key[i++] = c;
		}
	}

	value[i] = '\0';

	if (!have_key) {
		fprintf(stderr, "do not have key");
		return -1;
	}
	if (have_quote) {
		fprintf(stderr, "need another quote");
		return -1;
	}

	return rtl_config_add(key, value);
}

int rtl_config_load(const char *filename)
{
	FILE *fp;
	char line[1024];

	if (!(fp = fopen(filename, "r")))
		return -1;

	while (fgets(line, sizeof(line), fp)) {
		/* ignore lines that start with a comment or '\n' character */
		if (*line == comment || *line == '\n')
			continue;

		if (parse_line(line) < 0) {
			fclose(fp);
			rtl_config_free();
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

int rtl_config_save(const char *filename)
{
	FILE *fp;
	char line[1024];
	struct config_item *pos, *tmp;

	if (!items)
		return -1;

	if ((fp = fopen(filename, "w")) == NULL)
		return -1;

	RTL_HASH_ITER(hh, items, pos, tmp) {
		if (has_space(pos->value))
			sprintf(line, "%s %c \"%s\"\n", (char *)pos->key, delim, (char *)pos->value);
		else
			sprintf(line, "%s %c %s\n", (char *)pos->key, delim, (char *)pos->value);
		fputs(line, fp);
	}

	fclose(fp);
	return 0;
}

void rtl_config_free(void)
{
	struct config_item *pos, *tmp;

	RTL_HASH_ITER(hh, items, pos, tmp) {
		RTL_HASH_DEL(items, pos);
		free(pos->key);
		free(pos->value);
		free(pos);
	}
	items = NULL;
}
