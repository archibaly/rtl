#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <rtl_hash.h>

#define array_size(x)	(sizeof(x) / sizeof(x[0]))

int main()
{
	struct rtl_hash_table *table;
	table = rtl_hash_init(31, RTL_HASH_KEY_TYPE_STR);
	if (!table)
		return -1;

	rtl_hash_add(table, "123", sizeof("123"), "value1", sizeof("value1"));
	rtl_hash_add(table, "123", sizeof("123"), "valuex", sizeof("valuex"));
	rtl_hash_add(table, "123", sizeof("123"), "value3", sizeof("valuex"));
	rtl_hash_add(table, "123", sizeof("123") , "valuex", sizeof("valuex"));
	rtl_hash_add(table, "123", sizeof("123") , "valuex", sizeof("valuex"));
	rtl_hash_add(table, "aece", sizeof("aece") , "value2", sizeof("value2"));
	rtl_hash_add(table, "0uej", sizeof("0uej") , "value3", sizeof("value3"));

	struct rtl_hash_node *node[4];

	int n = rtl_hash_find(table, "123", node, array_size(node));
	printf("n = %d\n", n);

	int i;
	if (n > 0) {
		for (i = 0; i < n && i < array_size(node); i++)
			printf("node[%d]->value = %s\n", i, (char *)node[i]->value);
	} else {
		printf("can not find\n");
	}

	rtl_hash_destroy(table);

	table = rtl_hash_init(32, RTL_HASH_KEY_TYPE_INT);
	if (!table)
		return -1;

	int key = 1;
	rtl_hash_add(table, &key, sizeof(key), "value1", sizeof("value1"));
	key = 1;
	rtl_hash_add(table, &key, sizeof(key), "valuex", sizeof("valuex"));
	key = 3;
	rtl_hash_add(table, &key, sizeof(key), "value2", sizeof("value2"));
	key = 4;
	rtl_hash_add(table, &key, sizeof(key), "value3", sizeof("value3"));

	key = 1;
	n = rtl_hash_find(table, &key, node, array_size(node));

	if (n > 0) {
		for (i = 0; i < n && i < array_size(node); i++)
			printf("node[%d]->value = %s\n", i, (char *)node[i]->value);
	} else {
		printf("can not find\n");
	}

	rtl_hash_destroy(table);

	return 0;
}
