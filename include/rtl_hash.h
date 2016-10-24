#ifndef _RTL_HASH_H_
#define _RTL_HASH_H_

#include "rtl_list.h"

#define RTL_HASH_KEY_TYPE_INT	1
#define RTL_HASH_KEY_TYPE_STR	2

#define rtl_hash_for_each_entry(pos, head) hlist_for_each_entry(pos, head, node)
#define rtl_hash_for_each_entry_safe(pos, n, head) hlist_for_each_entry_safe(pos, n, head, node)
#define rtl_hash_head hlist_head

struct rtl_hash_node {
	void *key;
	void *value;
	struct hlist_node node;
};

struct rtl_hash_table {
	int size;
	int key_type;
	struct rtl_hash_head *head;
};

struct rtl_hash_table *rtl_hash_init(int size, int key_type);
int rtl_hash_add(struct rtl_hash_table *table, void *key, void *value);
int rtl_hash_find(struct rtl_hash_table *table, const void *key,
				  struct rtl_hash_node **node, size_t size);
void rtl_hash_del(struct rtl_hash_node *node);
void rtl_hash_free(struct rtl_hash_table *table);

#endif /* _RTL_HASH_H_ */
