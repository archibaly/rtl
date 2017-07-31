#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "rtl_dict.h"

/* Maximum value size for integers and doubles. */
#define MAXVALSZ    1024

/* Minimal allocated number of entries in a rtl_dict_t */
#define DICTMINSZ   128

/* Invalid key token */
#define DICT_INVALID_KEY    ((char*)-1)

/**
 * @brief    Duplicate a string
 * @param    s String to duplicate
 * @return   Pointer to a newly allocated string, to be freed with free()
 *
 * This is a replacement for strdup(). This implementation is provided
 * for systems that do not have it.
 */
static int dict_grow(rtl_dict_t *d)
{
	char **new_val;
	char **new_key;
	unsigned *new_hash;

	new_val = (char **)calloc(d->size *2, sizeof *d->val);
	new_key = (char **)calloc(d->size *2, sizeof *d->key);
	new_hash = (unsigned *)calloc(d->size *2, sizeof *d->hash);
	if (!new_val || !new_key || !new_hash) {
		/* An allocation failed, leave the rtl_dict_t unchanged */
		if (new_val)
			free(new_val);
		if (new_key)
			free(new_key);
		if (new_hash)
			free(new_hash);
		return -1;
	}
	/* Initialize the newly allocated space */
	memcpy(new_val, d->val, d->size *sizeof(char *));
	memcpy(new_key, d->key, d->size *sizeof(char *));
	memcpy(new_hash, d->hash, d->size *sizeof(unsigned));
	/* Delete previous data */
	free(d->val);
	free(d->key);
	free(d->hash);
	/* Actually update the rtl_dict_t */
	d->size *= 2;
	d->val = new_val;
	d->key = new_key;
	d->hash = new_hash;
	return 0;
}

unsigned rtl_dict_hash(const char *key)
{
	size_t len;
	unsigned hash;
	size_t i;

	if (!key)
		return 0;

	len = strlen(key);
	for (hash = 0, i = 0; i < len; i++) {
		hash += (unsigned)key[i];
		hash += (hash << 10);
		hash ^= (hash >> 6);
	}
	hash += (hash << 3);
	hash ^= (hash >> 11);
	hash += (hash << 15);
	return hash;
}

rtl_dict_t *rtl_dict_new(size_t size)
{
	rtl_dict_t *d;

	/* If no size was specified, allocate space for DICTMINSZ */
	if (size < DICTMINSZ)
		size = DICTMINSZ;

	d = (rtl_dict_t *) calloc(1, sizeof *d);

	if (d) {
		d->size = size;
		d->val = (char **)calloc(size, sizeof *d->val);
		d->key = (char **)calloc(size, sizeof *d->key);
		d->hash = (unsigned *)calloc(size, sizeof *d->hash);
	}
	return d;
}

void rtl_dict_del(rtl_dict_t *d)
{
	ssize_t i;

	if (d == NULL)
		return;
	for (i = 0; i < d->size; i++) {
		if (d->key[i] != NULL)
			free(d->key[i]);
		if (d->val[i] != NULL)
			free(d->val[i]);
	}
	free(d->val);
	free(d->key);
	free(d->hash);
	free(d);
	return;
}

const char *rtl_dict_get(const rtl_dict_t *d, const char *key,
						 const char *def)
{
	unsigned hash;
	ssize_t i;

	hash = rtl_dict_hash(key);
	for (i = 0; i < d->size; i++) {
		if (d->key[i] == NULL)
			continue;
		/* Compare hash */
		if (hash == d->hash[i]) {
			/* Compare string, to avoid hash collisions */
			if (!strcmp(key, d->key[i])) {
				return d->val[i];
			}
		}
	}
	return def;
}

int rtl_dict_set(rtl_dict_t *d, const char *key, const char *val)
{
	ssize_t i;
	unsigned hash;

	if (d == NULL || key == NULL)
		return -1;

	/* Compute hash for this key */
	hash = rtl_dict_hash(key);
	/* Find if value is already in rtl_dict_t */
	if (d->n > 0) {
		for (i = 0; i < d->size; i++) {
			if (d->key[i] == NULL)
				continue;
			if (hash == d->hash[i]) {	/* Same hash value */
				if (!strcmp(key, d->key[i])) {	/*Same key */
					/* Found a value: modify and return */
					if (d->val[i] != NULL)
						free(d->val[i]);
					d->val[i] = (val ? strdup(val) : NULL);
					/* Value has been modified: return */
					return 0;
				}
			}
		}
	}
	/* Add a new value */
	/* See if rtl_dict_t needs to grow */
	if (d->n == d->size) {
		/* Reached maximum size: reallocate rtl_dict_t */
		if (dict_grow(d) != 0)
			return -1;
	}

	/* Insert key in the first empty slot. Start at d->n and wrap at
	 * d->size. Because d->n < d->size this will necessarily
	 * terminate.
	 */
	for (i = d->n; d->key[i];) {
		if (++i == d->size)
			i = 0;
	}
	/* Copy key */
	d->key[i] = strdup(key);
	d->val[i] = (val ? strdup(val) : NULL);
	d->hash[i] = hash;
	d->n++;
	return 0;
}

void rtl_dict_unset(rtl_dict_t *d, const char *key)
{
	unsigned hash;
	ssize_t i;

	if (key == NULL || d == NULL) {
		return;
	}

	hash = rtl_dict_hash(key);
	for (i = 0; i < d->size; i++) {
		if (d->key[i] == NULL)
			continue;
		/* Compare hash */
		if (hash == d->hash[i]) {
			/* Compare string, to avoid hash collisions */
			if (!strcmp(key, d->key[i])) {
				/* Found key */
				break;
			}
		}
	}
	if (i >= d->size)
		/* Key not found */
		return;

	free(d->key[i]);
	d->key[i] = NULL;
	if (d->val[i] != NULL) {
		free(d->val[i]);
		d->val[i] = NULL;
	}
	d->hash[i] = 0;
	d->n--;
	return;
}

void rtl_dict_dump(const rtl_dict_t *d, FILE *out)
{
	ssize_t i;

	if (d == NULL || out == NULL)
		return;
	if (d->n < 1) {
		fprintf(out, "empty dictionary\n");
		return;
	}
	for (i = 0; i < d->size; i++) {
		if (d->key[i]) {
			fprintf(out, "%20s\t[%s]\n",
					d->key[i], d->val[i] ? d->val[i] : "UNDEF");
		}
	}
	return;
}
