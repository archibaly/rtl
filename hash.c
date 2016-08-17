#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "hash.h"
#include "debug.h"

/* 2^31 + 2^29 - 2^25 + 2^22 - 2^19 - 2^16 + 1 */
#define GOLDEN_RATIO_PRIME_32 0x9e370001UL

static inline uint32_t next_power_of_two(uint32_t x)
{
	return 1 << (32 - __builtin_clz(x - 1));
}

static uint32_t hash_int(uint32_t val, uint32_t bits)
{
	uint32_t hash = val * GOLDEN_RATIO_PRIME_32;

	/* high bits are more random, so use them */
	return hash >> (32 - bits);
}

static uint32_t hash_str(const char *val, uint32_t size)
{
	uint32_t hash = 0;
	uint32_t seed = 131;	/* 31 131 1313 13131 131313 .. */

	while (*val)
		hash = hash * seed + (uint32_t)*val++;

	return hash % size;
}

struct hash_table *hash_init(int size, int key_type)
{
	int i;
	struct hash_table *table;

	if (!(table = malloc(sizeof(struct hash_table))))
		return NULL;

	table->size = next_power_of_two(size);
	table->key_type = key_type;
	if (!(table->head = malloc(sizeof(struct hash_head) * table->size))) {
		free(table);
		return NULL;
	}

	for (i = 0; i < table->size; i++)
		INIT_HLIST_HEAD(table->head + i);

	return table;
}

static struct hash_node *new_hash_node(void *key, void *value)
{
	struct hash_node *node;

	if (!(node = malloc(sizeof(struct hash_node))))
		return NULL;

	node->key = key;
	node->value = value;
	hlist_node_init(&node->node);

	return node;
}

int hash_add(struct hash_table *table, void *key, void *value)
{
	int offset;
	struct hash_node *node;

	if (table->key_type == HASH_KEY_TYPE_INT) {
		offset = hash_int(*(int *)key, log(table->size) / log(2));
	} else if (table->key_type == HASH_KEY_TYPE_STR) {
		offset = hash_str((char *)key, table->size);
	} else {
		return -1;
	}

	if (!(node = new_hash_node(key, value)))
		return -1;

	hlist_add_head(&node->node, table->head + offset);

	return 0;
}

static int hash_int_find(struct hash_table *table, int key,
						 struct hash_node **node, size_t size)
{
	int offset;
	size_t i = 0;
	struct hash_node *pos;

	if (!table)
		return 0;

	offset = hash_int(key, log(table->size) / log(2));

	hash_for_each_entry(pos, table->head + offset) {
		if (*(int *)pos->key == key) {
			if (i < size)
				node[i++] = pos;
			else
				break;
		}
	}

	return i;
}

static int hash_str_find(struct hash_table *table, const char *key,
						 struct hash_node **node, size_t size)
{
	int offset;
	size_t i = 0;
	struct hash_node *pos;

	if (!table)
		return 0;

	offset = hash_str(key, table->size);

	hash_for_each_entry(pos, table->head + offset) {
		if (strcmp((char *)pos->key, key) == 0) {
			if (i < size)
				node[i++] = pos;
			else
				break;
		}
	}

	return i;
}

/*
 * @return: the number of found nodes
 */
int hash_find(struct hash_table *table, const void *key,
			  struct hash_node **node, size_t size)
{
	if (table->key_type == HASH_KEY_TYPE_INT) {
		return hash_int_find(table, *(int *)key, node, size);
	} else if (table->key_type == HASH_KEY_TYPE_STR) {
		return hash_str_find(table, (char *)key, node, size);
	} else {
		return 0;
	}
}

void hash_free_node(struct hash_node *node)
{
	if (!node)
		return;
	if (!hlist_unhashed(&node->node)) {
		__hlist_del(&node->node);
	}
	free(node);
}

void hash_free(struct hash_table *table)
{
	int i;
	struct hash_node *pos;

	if (!table)
		return;

	for (i = 0; i < table->size; i++) {
		hash_for_each_entry(pos, table->head + i) {
			hash_free_node(pos);
		}
	}

	free(table->head);
	free(table);
}
