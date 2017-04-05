#include <stdio.h>

#include <rtl_hash.h>

#define array_size(x)	(sizeof(x) / sizeof(x[0]))

int main()
{
	struct rtl_hash_table *table;
	table = rtl_hash_init(31, RTL_HASH_KEY_TYPE_STR);
	if (!table)
		return -1;

	rtl_hash_add(table, "123", "value1");
	rtl_hash_add(table, "123", "valuex");
	rtl_hash_add(table, "123", "value3");
	rtl_hash_add(table, "123", "valuex");
	rtl_hash_add(table, "123", "valuex");
	rtl_hash_add(table, "aece", "value2");
	rtl_hash_add(table, "0uej", "value3");

	struct rtl_hash_node *node[4];

	int n = rtl_hash_find(table, "123", node, sizeof(node));
	printf("n = %d\n", n);

	int i;
	if (n > 0) {
		for (i = 0; i < n && i < array_size(node); i++)
			printf("node[%d]->value = %s\n", i, (char *)node[i]->value);
	} else {
		printf("can not find\n");
	}

	rtl_hash_free(table);

	table = rtl_hash_init(32, RTL_HASH_KEY_TYPE_INT);
	if (!table)
		return -1;

	int key = 1;
	rtl_hash_add(table, &key, "value1");
	key = 1;
	rtl_hash_add(table, &key, "valuex");
	key = 3;
	rtl_hash_add(table, &key, "value2");
	key = 4;
	rtl_hash_add(table, &key, "value3");

	key = 1;
	n = rtl_hash_find(table, &key, node, sizeof(node));

	if (n > 0) {
		for (i = 0; i < n && i < array_size(node); i++)
			printf("node[%d]->value = %s\n", i, (char *)node[i]->value);
	} else {
		printf("can not find\n");
	}

	rtl_hash_free(table);

	return 0;
}
