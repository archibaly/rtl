#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <rtl_rbtree.h>

#define NUM_NODES 32

struct mynode {
  	struct rtl_rb_node node;
  	char *string;
};

struct rtl_rb_root mytree = RTL_RB_ROOT;

struct mynode *my_search(struct rtl_rb_root *root, char *string)
{
  	struct rtl_rb_node *node = root->rb_node;

  	while (node) {
  		struct mynode *data = rtl_rb_entry(node, struct mynode, node);
		int result;

		result = strcmp(string, data->string);

		if (result < 0)
  			node = node->rb_left;
		else if (result > 0)
  			node = node->rb_right;
		else
  			return data;
	}
	return NULL;
}

int my_insert(struct rtl_rb_root *root, struct mynode *data)
{
  	struct rtl_rb_node **new = &(root->rb_node), *parent = NULL;

  	/* Figure out where to put new node */
  	while (*new) {
  		struct mynode *this = rtl_rb_entry(*new, struct mynode, node);
  		int result = strcmp(data->string, this->string);

		parent = *new;
  		if (result < 0)
  			new = &((*new)->rb_left);
  		else if (result > 0)
  			new = &((*new)->rb_right);
  		else
  			return 0;
  	}

  	/* Add new node and rebalance tree. */
  	rtl_rb_link_node(&data->node, parent, new);
  	rtl_rb_insert_color(&data->node, root);

	return 0;
}

void my_free(struct mynode *node)
{
	if (node != NULL) {
		if (node->string != NULL) {
			free(node->string);
			node->string = NULL;
		}
		free(node);
		node = NULL;
	}
}

int main()
{
	struct mynode *mn[NUM_NODES];

	/* *insert */
	int i = 0;
	printf("insert node from 1 to NUM_NODES(32): \n");
	for (; i < NUM_NODES; i++) {
		mn[i] = (struct mynode *)malloc(sizeof(struct mynode));
		mn[i]->string = (char *)malloc(sizeof(char) * 4);
		sprintf(mn[i]->string, "%d", i);
		my_insert(&mytree, mn[i]);
	}

	/* *search */
	struct rtl_rb_node *node;
	printf("search all nodes: \n");
	for (node = rtl_rb_first(&mytree); node; node = rtl_rb_next(node))
		printf("key = %s\n", rtl_rb_entry(node, struct mynode, node)->string);

	/* *delete */
	printf("delete node 20: \n");
	struct mynode *data = my_search(&mytree, "20");
	if (data) {
		rtl_rb_erase(&data->node, &mytree);
		my_free(data);
	}

	/* *delete again*/
	printf("delete node 10: \n");
	data = my_search(&mytree, "10");
	if (data) {
		rtl_rb_erase(&data->node, &mytree);
		my_free(data);
	}

	/* *delete once again*/
	printf("delete node 15: \n");
	data = my_search(&mytree, "15");
	if (data) {
		rtl_rb_erase(&data->node, &mytree);
		my_free(data);
	}

	/* *search again*/
	printf("search again:\n");
	for (node = rtl_rb_first(&mytree); node; node = rtl_rb_next(node))
		printf("key = %s\n", rtl_rb_entry(node, struct mynode, node)->string);
	return 0;
}
