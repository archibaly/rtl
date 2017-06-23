#ifndef	_RTL_RBTREE_H_
#define	_RTL_RBTREE_H_

#include "rtl_list.h"

struct rtl_rb_node {
	unsigned long rb_parent_color;
#define	RTL_RB_RED		0
#define	RTL_RB_BLACK	1
	struct rtl_rb_node *rb_right;
	struct rtl_rb_node *rb_left;
} __attribute__ ((aligned(sizeof(long))));

/* The alignment might seem pointless, but allegedly CRIS needs it */

struct rtl_rb_root {
	struct rtl_rb_node *rb_node;
};

#define rtl_rb_parent(r)	((struct rtl_rb_node *)((r)->rb_parent_color & ~3))
#define rtl_rb_color(r)		((r)->rb_parent_color & 1)
#define rtl_rb_is_red(r)	(!rtl_rb_color(r))
#define rtl_rb_is_black(r)	(rtl_rb_color(r))
#define rtl_rb_set_red(r)	do { (r)->rb_parent_color &= ~1; } while (0)
#define rtl_rb_set_black(r)	do { (r)->rb_parent_color |= 1; } while (0)

static inline void rtl_rb_set_parent(struct rtl_rb_node *rb, struct rtl_rb_node *p)
{
	rb->rb_parent_color = (rb->rb_parent_color & 3) | (unsigned long)p;
}

static inline void rtl_rb_set_color(struct rtl_rb_node *rb, int color)
{
	rb->rb_parent_color = (rb->rb_parent_color & ~1) | color;
}

#define RTL_RB_ROOT	(struct rtl_rb_root) { NULL, }
#define	rtl_rb_entry(ptr, type, member) rtl_container_of(ptr, type, member)

#define RTL_RB_EMPTY_ROOT(root)	((root)->rb_node == NULL)
#define RTL_RB_EMPTY_NODE(node)	(rtl_rb_parent(node) == node)
#define RTL_RB_CLEAR_NODE(node)	(rtl_rb_set_parent(node, node))

static inline void rtl_rb_init_node(struct rtl_rb_node *rb)
{
	rb->rb_parent_color = 0;
	rb->rb_right = NULL;
	rb->rb_left = NULL;
	RTL_RB_CLEAR_NODE(rb);
}

void rtl_rb_insert_color(struct rtl_rb_node *, struct rtl_rb_root *);
void rtl_rb_erase(struct rtl_rb_node *, struct rtl_rb_root *);

typedef void (*rb_augment_f) (struct rtl_rb_node * node, void *data);

void rtl_rb_augment_insert(struct rtl_rb_node *node,
							  rb_augment_f func, void *data);
struct rtl_rb_node *rtl_rb_augment_erase_begin(struct rtl_rb_node *node);
void rtl_rb_augment_erase_end(struct rtl_rb_node *node,
								 rb_augment_f func, void *data);

/* Find logical next and previous nodes in a tree */
struct rtl_rb_node *rtl_rb_next(const struct rtl_rb_node *);
struct rtl_rb_node *rtl_rb_prev(const struct rtl_rb_node *);
struct rtl_rb_node *rtl_rb_first(const struct rtl_rb_root *);
struct rtl_rb_node *rtl_rb_last(const struct rtl_rb_root *);

/* Fast replacement of a single node without remove/rebalance/add/rebalance */
void rb_replace_node(struct rtl_rb_node *victim, struct rtl_rb_node *new,
							struct rtl_rb_root *root);

static inline void rtl_rb_link_node(struct rtl_rb_node *node, struct rtl_rb_node *parent,
								struct rtl_rb_node **rb_link)
{
	node->rb_parent_color = (unsigned long)parent;
	node->rb_left = node->rb_right = NULL;

	*rb_link = node;
}

#endif /* _RTL_RBTREE_H_ */
