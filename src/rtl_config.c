#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

#include "rtl_hash.h"
#include "rtl_debug.h"
#include "rtl_config.h"

#define END_LINE(c)			(c == '\n' || c == '\0')
#define HASH_NUM_BUCKETS	37

static char delim = '=';
static char comment = '#';

static struct rtl_hash_table *config_table;

int rtl_config_add(const char *key, const char *value)
{
	int n;
	struct rtl_hash_node *node;

	n = rtl_hash_find(config_table, key, &node, 1);

	if (n > 0) {
		if (rtl_hash_del(node) < 0)
			return -1;
	}

	return rtl_hash_add(config_table, key, strlen(key) + 1, value, strlen(value) + 1);
}

void rtl_config_del(const char *key)
{
	struct rtl_hash_node *node;

	if (rtl_hash_find(config_table, key, &node, 1) == 0)
		return;
	rtl_hash_del(node);
}

char *rtl_config_get_value(const char *key)
{
	struct rtl_hash_node *node;
	int n = rtl_hash_find(config_table, key, &node, 1);

	if (n == 0)
		return NULL;
	else
		return (char *)node->value;
}

void rtl_config_set_delim(char d)
{
	delim = d;
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
				rtl_debug("unexpected '%c'", '"');
				return -1;
			}
			if (have_quote && !END_LINE(*string)) {
				rtl_debug("unexpected '%c' after '%c'", *string, '"');
				return -1;
			}
			have_quote = !have_quote;
		} else if (c == ' ') {
			/* ignore spaces outside of quotes. */
			if (have_quote)
				value[i++] = c;
		} else if (c == delim) {
			if (have_key) {
				rtl_debug("unexpected '%c'", delim);
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
		rtl_debug("do not have key");
		return -1;
	}
	if (have_quote) {
		rtl_debug("need another quote");
		return -1;
	}

	return rtl_config_add(key, value);
}

int rtl_config_load(const char *filename)
{
	FILE *fp;
	char line[1024];

	if (!(config_table = rtl_hash_init(HASH_NUM_BUCKETS, RTL_HASH_KEY_TYPE_STR)))
		return -1;

	if (!(fp = fopen(filename, "r"))) {
		rtl_hash_destroy(config_table);
		return -1;
	}

	while (fgets(line, sizeof(line), fp)) {
		/* ignore lines that start with a comment or '\n' character */
		if (*line == comment || *line == '\n')
			continue;

		if (parse_line(line) < 0) {
			fclose(fp);
			rtl_hash_destroy(config_table);
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

	if (!config_table)
		return -1;

	if ((fp = fopen(filename, "w")) == NULL)
		return -1;

	int i;
	struct rtl_hash_node *pos;

	for (i = 0; i < HASH_NUM_BUCKETS; i++) {
		rtl_hash_for_each_entry(pos, config_table->head + i) {
			if (has_space(pos->value))
				sprintf(line, "%s %c \"%s\"\n", (char *)pos->key, delim, (char *)pos->value);
			else
				sprintf(line, "%s %c %s\n", (char *)pos->key, delim, (char *)pos->value);
			fputs(line, fp);
		}
	}

	fclose(fp);
	return 0;
}

void rtl_config_free(void)
{
	rtl_hash_destroy(config_table);
	config_table = NULL;
}
