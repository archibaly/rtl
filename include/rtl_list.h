#ifndef _RTL_LIST_H_
#define _RTL_LIST_H_

#include <stddef.h>

#define rtl_container_of(ptr, type, member) ({	\
		const typeof( ((type *)0)->member ) *__mptr = (ptr);	\
		(type *)( (char *)__mptr - offsetof(type,member) );})

/*
 * Simple doubly linked list implementation.
 *
 * Some of the internal functions ("__xxx") are useful when
 * manipulating whole lists rather than single entries, as
 * sometimes we already know the next/prev entries and we can
 * generate better code by using them directly rather than
 * using the generic single-entry routines.
 */

struct rtl_list_head {
	struct rtl_list_head *prev, *next;
};

#define RTL_LIST_HEAD_INIT(name) { &(name), &(name) }

#define RTL_LIST_HEAD(name) \
	struct rtl_list_head name = RTL_LIST_HEAD_INIT(name)

static inline void rtl_list_head_init(struct rtl_list_head *list)
{
	list->next = list;
	list->prev = list;
}

/*
 * Insert a new entry between two known consecutive entries.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
static inline void __list_add(struct rtl_list_head *new,
							  struct rtl_list_head *prev, struct rtl_list_head *next)
{
	next->prev = new;
	new->next = next;
	new->prev = prev;
	prev->next = new;
}

/**
 * rtl_list_add - add a new entry
 * @new: new entry to be added
 * @head: list head to add it after
 *
 * Insert a new entry after the specified head.
 * This is good for implementing stacks.
 */
static inline void rtl_list_add(struct rtl_list_head *new, struct rtl_list_head *head)
{
	__list_add(new, head, head->next);
}

static inline struct rtl_list_head *rtl_list_get_tail(struct rtl_list_head *head)
{
	return head->prev;
}

/**
 * rtl_list_add_tail - add a new entry
 * @new: new entry to be added
 * @head: list head to add it before
 *
 * Insert a new entry before the specified head.
 * This is useful for implementing queues.
 */
static inline void rtl_list_add_tail(struct rtl_list_head *new, struct rtl_list_head *head)
{
	__list_add(new, head->prev, head);
}

/*
 * Delete a list entry by making the prev/next entries
 * point to each other.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
static inline void __list_del(struct rtl_list_head *prev, struct rtl_list_head *next)
{
	next->prev = prev;
	prev->next = next;
}

/**
 * rtl_list_del - deletes entry from list.
 * @entry: the element to delete from the list.
 * Note: rtl_list_empty() on entry does not return true after this, the entry is
 * in an undefined state.
 */
static inline void __list_del_entry(struct rtl_list_head *entry)
{
	__list_del(entry->prev, entry->next);
}

static inline void rtl_list_del(struct rtl_list_head *entry)
{
	__list_del(entry->prev, entry->next);
	entry->next = NULL;
	entry->prev = NULL;
}

/**
 * rtl_list_replace - replace old entry by new one
 * @old : the element to be replaced
 * @new : the new element to insert
 *
 * If @old was empty, it will be overwritten.
 */
static inline void rtl_list_replace(struct rtl_list_head *old, struct rtl_list_head *new)
{
	new->next = old->next;
	new->next->prev = new;
	new->prev = old->prev;
	new->prev->next = new;
}

static inline void list_replace_init(struct rtl_list_head *old,
									 struct rtl_list_head *new)
{
	rtl_list_replace(old, new);
	rtl_list_head_init(old);
}

/**
 * rtl_list_del_init - deletes entry from list and reinitialize it.
 * @entry: the element to delete from the list.
 */
static inline void rtl_list_del_init(struct rtl_list_head *entry)
{
	__list_del_entry(entry);
	rtl_list_head_init(entry);
}

/**
 * rtl_list_move - delete from one list and add as another's head
 * @list: the entry to move
 * @head: the head that will precede our entry
 */
static inline void rtl_list_move(struct rtl_list_head *list, struct rtl_list_head *head)
{
	__list_del_entry(list);
	rtl_list_add(list, head);
}

/**
 * rtl_list_move_tail - delete from one list and add as another's tail
 * @list: the entry to move
 * @head: the head that will follow our entry
 */
static inline void rtl_list_move_tail(struct rtl_list_head *list,
								  struct rtl_list_head *head)
{
	__list_del_entry(list);
	rtl_list_add_tail(list, head);
}

/**
 * rtl_list_is_last - tests whether @list is the last entry in list @head
 * @list: the entry to test
 * @head: the head of the list
 */
static inline int rtl_list_is_last(const struct rtl_list_head *list,
							   const struct rtl_list_head *head)
{
	return list->next == head;
}

/**
 * rtl_list_empty - tests whether a list is empty
 * @head: the list to test.
 */
static inline int rtl_list_empty(const struct rtl_list_head *head)
{
	return head->next == head;
}

/**
 * rtl_list_empty_careful - tests whether a list is empty and not being modified
 * @head: the list to test
 *
 * Description:
 * tests whether a list is empty _and_ checks that no other CPU might be
 * in the process of modifying either member (next or prev)
 *
 * NOTE: using rtl_list_empty_careful() without synchronization
 * can only be safe if the only activity that can happen
 * to the list entry is rtl_list_del_init(). Eg. it cannot be used
 * if another CPU could re-rtl_list_add() it.
 */
static inline int rtl_list_empty_careful(const struct rtl_list_head *head)
{
	struct rtl_list_head *next = head->next;

	return (next == head) && (next == head->prev);
}

/**
 * rtl_list_rotate_left - rotate the list to the left
 * @head: the head of the list
 */
static inline void rtl_list_rotate_left(struct rtl_list_head *head)
{
	struct rtl_list_head *first;

	if (!rtl_list_empty(head)) {
		first = head->next;
		rtl_list_move_tail(first, head);
	}
}

/**
 * rtl_list_is_singular - tests whether a list has just one entry.
 * @head: the list to test.
 */
static inline int rtl_list_is_singular(const struct rtl_list_head *head)
{
	return !rtl_list_empty(head) && (head->next == head->prev);
}

static inline void __list_cut_position(struct rtl_list_head *list,
									   struct rtl_list_head *head,
									   struct rtl_list_head *entry)
{
	struct rtl_list_head *new_first = entry->next;

	list->next = head->next;
	list->next->prev = list;
	list->prev = entry;
	entry->next = list;
	head->next = new_first;
	new_first->prev = head;
}

/**
 * rtl_list_cut_position - cut a list into two
 * @list: a new list to add all removed entries
 * @head: a list with entries
 * @entry: an entry within head, could be the head itself
 *	and if so we won't cut the list
 *
 * This helper moves the initial part of @head, up to and
 * including @entry, from @head to @list. You should
 * pass on @entry an element you know is on @head. @list
 * should be an empty list or a list you do not care about
 * losing its data.
 *
 */
static inline void rtl_list_cut_position(struct rtl_list_head *list,
									 struct rtl_list_head *head,
									 struct rtl_list_head *entry)
{
	if (rtl_list_empty(head))
		return;
	if (rtl_list_is_singular(head) && (head->next != entry && head != entry))
		return;
	if (entry == head)
		rtl_list_head_init(list);
	else
		__list_cut_position(list, head, entry);
}

static inline void __list_splice(const struct rtl_list_head *list,
								 struct rtl_list_head *prev, struct rtl_list_head *next)
{
	struct rtl_list_head *first = list->next;
	struct rtl_list_head *last = list->prev;

	first->prev = prev;
	prev->next = first;

	last->next = next;
	next->prev = last;
}

/**
 * rtl_list_splice - join two lists, this is designed for stacks
 * @list: the new list to add.
 * @head: the place to add it in the first list.
 */
static inline void rtl_list_splice(const struct rtl_list_head *list,
							   struct rtl_list_head *head)
{
	if (!rtl_list_empty(list))
		__list_splice(list, head, head->next);
}

/**
 * rtl_list_splice_tail - join two lists, each list being a queue
 * @list: the new list to add.
 * @head: the place to add it in the first list.
 */
static inline void rtl_list_splice_tail(struct rtl_list_head *list,
									struct rtl_list_head *head)
{
	if (!rtl_list_empty(list))
		__list_splice(list, head->prev, head);
}

/**
 * rtl_list_splice_init - join two lists and reinitialise the emptied list.
 * @list: the new list to add.
 * @head: the place to add it in the first list.
 *
 * The list at @list is reinitialised
 */
static inline void rtl_list_splice_init(struct rtl_list_head *list,
									struct rtl_list_head *head)
{
	if (!rtl_list_empty(list)) {
		__list_splice(list, head, head->next);
		rtl_list_head_init(list);
	}
}

/**
 * rtl_list_splice_tail_init - join two lists and reinitialise the emptied list
 * @list: the new list to add.
 * @head: the place to add it in the first list.
 *
 * Each of the lists is a queue.
 * The list at @list is reinitialised
 */
static inline void rtl_list_splice_tail_init(struct rtl_list_head *list,
										 struct rtl_list_head *head)
{
	if (!rtl_list_empty(list)) {
		__list_splice(list, head->prev, head);
		rtl_list_head_init(list);
	}
}

/**
 * rtl_list_entry - get the struct for this entry
 * @ptr:	the &struct rtl_list_head pointer.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the rtl_list_head within the struct.
 */
#define rtl_list_entry(ptr, type, member) \
	rtl_container_of(ptr, type, member)

/**
 * rtl_list_first_entry - get the first element from a list
 * @ptr:	the list head to take the element from.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the rtl_list_head within the struct.
 *
 * Note, that list is expected to be not empty.
 */
#define rtl_list_first_entry(ptr, type, member) \
	rtl_list_entry((ptr)->next, type, member)

/**
 * rtl_list_last_entry - get the last element from a list
 * @ptr:	the list head to take the element from.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the rtl_list_head within the struct.
 *
 * Note, that list is expected to be not empty.
 */
#define rtl_list_last_entry(ptr, type, member) \
	rtl_list_entry((ptr)->prev, type, member)

/**
 * rtl_list_first_entry_or_null - get the first element from a list
 * @ptr:	the list head to take the element from.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the rtl_list_head within the struct.
 *
 * Note that if the list is empty, it returns NULL.
 */
#define rtl_list_first_entry_or_null(ptr, type, member) \
	(!rtl_list_empty(ptr) ? rtl_list_first_entry(ptr, type, member) : NULL)

/**
 * rtl_list_next_entry - get the next element in list
 * @pos:	the type * to cursor
 * @member:	the name of the rtl_list_head within the struct.
 */
#define rtl_list_next_entry(pos, member) \
	rtl_list_entry((pos)->member.next, typeof(*(pos)), member)

/**
 * rtl_list_prev_entry - get the prev element in list
 * @pos:	the type * to cursor
 * @member:	the name of the rtl_list_head within the struct.
 */
#define rtl_list_prev_entry(pos, member) \
	rtl_list_entry((pos)->member.prev, typeof(*(pos)), member)

/**
 * rtl_list_for_each	-	iterate over a list
 * @pos:	the &struct rtl_list_head to use as a loop cursor.
 * @head:	the head for your list.
 */
#define rtl_list_for_each(pos, head) \
	for (pos = (head)->next; pos != (head); pos = pos->next)

/**
 * rtl_list_for_each_prev	-	iterate over a list backwards
 * @pos:	the &struct rtl_list_head to use as a loop cursor.
 * @head:	the head for your list.
 */
#define rtl_list_for_each_prev(pos, head) \
	for (pos = (head)->prev; pos != (head); pos = pos->prev)

/**
 * rtl_list_for_each_safe - iterate over a list safe against removal of list entry
 * @pos:	the &struct rtl_list_head to use as a loop cursor.
 * @n:		another &struct rtl_list_head to use as temporary storage
 * @head:	the head for your list.
 */
#define rtl_list_for_each_safe(pos, n, head) \
	for (pos = (head)->next, n = pos->next; pos != (head); \
			pos = n, n = pos->next)

/**
 * rtl_list_for_each_prev_safe - iterate over a list backwards safe against removal of list entry
 * @pos:	the &struct rtl_list_head to use as a loop cursor.
 * @n:		another &struct rtl_list_head to use as temporary storage
 * @head:	the head for your list.
 */
#define rtl_list_for_each_prev_safe(pos, n, head) \
	for (pos = (head)->prev, n = pos->prev; \
			pos != (head); \
			pos = n, n = pos->prev)

/**
 * rtl_list_for_each_entry	-	iterate over list of given type
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your list.
 * @member:	the name of the rtl_list_head within the struct.
 */
#define rtl_list_for_each_entry(pos, head, member)				\
	for (pos = rtl_list_first_entry(head, typeof(*pos), member);	\
			&pos->member != (head);					\
			pos = rtl_list_next_entry(pos, member))

/**
 * rtl_list_for_each_entry_reverse - iterate backwards over list of given type.
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your list.
 * @member:	the name of the rtl_list_head within the struct.
 */
#define rtl_list_for_each_entry_reverse(pos, head, member)			\
	for (pos = rtl_list_last_entry(head, typeof(*pos), member);		\
			&pos->member != (head); 					\
			pos = rtl_list_prev_entry(pos, member))

/**
 * rtl_list_prepare_entry - prepare a pos entry for use in rtl_list_for_each_entry_continue()
 * @pos:	the type * to use as a start point
 * @head:	the head of the list
 * @member:	the name of the rtl_list_head within the struct.
 *
 * Prepares a pos entry for use as a start point in rtl_list_for_each_entry_continue().
 */
#define rtl_list_prepare_entry(pos, head, member) \
	((pos) ? : rtl_list_entry(head, typeof(*pos), member))

/**
 * rtl_list_for_each_entry_continue - continue iteration over list of given type
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your list.
 * @member:	the name of the rtl_list_head within the struct.
 *
 * Continue to iterate over list of given type, continuing after
 * the current position.
 */
#define rtl_list_for_each_entry_continue(pos, head, member) 		\
	for (pos = rtl_list_next_entry(pos, member);			\
			&pos->member != (head);					\
			pos = rtl_list_next_entry(pos, member))

/**
 * rtl_list_for_each_entry_continue_reverse - iterate backwards from the given point
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your list.
 * @member:	the name of the rtl_list_head within the struct.
 *
 * Start to iterate over list of given type backwards, continuing after
 * the current position.
 */
#define rtl_list_for_each_entry_continue_reverse(pos, head, member)		\
	for (pos = rtl_list_prev_entry(pos, member);			\
			&pos->member != (head);					\
			pos = rtl_list_prev_entry(pos, member))

/**
 * rtl_list_for_each_entry_from - iterate over list of given type from the current point
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your list.
 * @member:	the name of the rtl_list_head within the struct.
 *
 * Iterate over list of given type, continuing from current position.
 */
#define rtl_list_for_each_entry_from(pos, head, member) \
	for (; &pos->member != (head);					\
			pos = rtl_list_next_entry(pos, member))

/**
 * rtl_list_for_each_entry_safe - iterate over list of given type safe against removal of list entry
 * @pos:	the type * to use as a loop cursor.
 * @n:		another type * to use as temporary storage
 * @head:	the head for your list.
 * @member:	the name of the rtl_list_head within the struct.
 */
#define rtl_list_for_each_entry_safe(pos, n, head, member)			\
	for (pos = rtl_list_first_entry(head, typeof(*pos), member),	\
			n = rtl_list_next_entry(pos, member);					\
			&pos->member != (head); 							\
			pos = n, n = rtl_list_next_entry(n, member))

/**
 * rtl_list_for_each_entry_safe_continue - continue list iteration safe against removal
 * @pos:	the type * to use as a loop cursor.
 * @n:		another type * to use as temporary storage
 * @head:	the head for your list.
 * @member:	the name of the rtl_list_head within the struct.
 *
 * Iterate over list of given type, continuing after current point,
 * safe against removal of list entry.
 */
#define rtl_list_for_each_entry_safe_continue(pos, n, head, member) \
	for (pos = rtl_list_next_entry(pos, member), 					\
			n = rtl_list_next_entry(pos, member);					\
			&pos->member != (head);								\
			pos = n, n = rtl_list_next_entry(n, member))

/**
 * rtl_list_for_each_entry_safe_from - iterate over list from current point safe against removal
 * @pos:	the type * to use as a loop cursor.
 * @n:		another type * to use as temporary storage
 * @head:	the head for your list.
 * @member:	the name of the rtl_list_head within the struct.
 *
 * Iterate over list of given type from current point, safe against
 * removal of list entry.
 */
#define rtl_list_for_each_entry_safe_from(pos, n, head, member)	\
	for (n = rtl_list_next_entry(pos, member);					\
			&pos->member != (head);							\
			pos = n, n = rtl_list_next_entry(n, member))

/**
 * rtl_list_for_each_entry_safe_reverse - iterate backwards over list safe against removal
 * @pos:	the type * to use as a loop cursor.
 * @n:		another type * to use as temporary storage
 * @head:	the head for your list.
 * @member:	the name of the rtl_list_head within the struct.
 *
 * Iterate backwards over list of given type, safe against removal
 * of list entry.
 */
#define rtl_list_for_each_entry_safe_reverse(pos, n, head, member)	\
	for (pos = rtl_list_last_entry(head, typeof(*pos), member),		\
			n = rtl_list_prev_entry(pos, member);					\
			&pos->member != (head); 							\
			pos = n, n = rtl_list_prev_entry(n, member))

/**
 * rtl_list_safe_reset_next - reset a stale rtl_list_for_each_entry_safe loop
 * @pos:	the loop cursor used in the rtl_list_for_each_entry_safe loop
 * @n:		temporary storage used in rtl_list_for_each_entry_safe
 * @member:	the name of the rtl_list_head within the struct.
 *
 * rtl_list_safe_reset_next is not safe to use in general if the list may be
 * modified concurrently (eg. the lock is dropped in the loop body). An
 * exception to this is if the cursor element (pos) is pinned in the list,
 * and rtl_list_safe_reset_next is called after re-taking the lock and before
 * completing the current iteration of the loop body.
 */
#define rtl_list_safe_reset_next(pos, n, member)	\
	n = rtl_list_next_entry(pos, member)

/*
 * Double linked lists with a single pointer list head.
 * Mostly useful for hash tables where the two pointer list head is
 * too wasteful.
 * You lose the ability to access the tail in O(1).
 */

struct rtl_hlist_head {
	struct rtl_hlist_node *first;
};

struct rtl_hlist_node {
	struct rtl_hlist_node *next, **pprev;
};

#define RTL_HLIST_HEAD_INIT { .first = NULL }
#define RTL_HLIST_HEAD(name) struct rtl_hlist_head name = {  .first = NULL }
#define RTL_INIT_HLIST_HEAD(ptr) ((ptr)->first = NULL)
static inline void rtl_hlist_node_init(struct rtl_hlist_node *h)
{
	h->next = NULL;
	h->pprev = NULL;
}

static inline int rtl_hlist_unhashed(const struct rtl_hlist_node *h)
{
	return !h->pprev;
}

static inline int rtl_hlist_empty(const struct rtl_hlist_head *h)
{
	return !h->first;
}

static inline void __hlist_del(struct rtl_hlist_node *n)
{
	struct rtl_hlist_node *next = n->next;
	struct rtl_hlist_node **pprev = n->pprev;

	*pprev = next;
	if (next)
		next->pprev = pprev;
}

static inline void rtl_hlist_del(struct rtl_hlist_node *n)
{
	__hlist_del(n);
	n->next = NULL;
	n->pprev = NULL;
}

static inline void rtl_hlist_del_init(struct rtl_hlist_node *n)
{
	if (!rtl_hlist_unhashed(n)) {
		__hlist_del(n);
		rtl_hlist_node_init(n);
	}
}

static inline void rtl_hlist_add_head(struct rtl_hlist_node *n, struct rtl_hlist_head *h)
{
	struct rtl_hlist_node *first = h->first;

	n->next = first;
	if (first)
		first->pprev = &n->next;
	h->first = n;
	n->pprev = &h->first;
}

/* next must be != NULL */
static inline void rtl_hlist_add_before(struct rtl_hlist_node *n,
										struct rtl_hlist_node *next)
{
	n->pprev = next->pprev;
	n->next = next;
	next->pprev = &n->next;
	*(n->pprev) = n;
}

static inline void rlt_hlist_add_behind(struct rtl_hlist_node *n,
										struct rtl_hlist_node *prev)
{
	n->next = prev->next;
	prev->next = n;
	n->pprev = &prev->next;

	if (n->next)
		n->next->pprev = &n->next;
}

/* after that we'll appear to be on some hlist and rtl_hlist_del will work */
static inline void rtl_hlist_add_fake(struct rtl_hlist_node *n)
{
	n->pprev = &n->next;
}

static inline int rtl_hlist_fake(struct rtl_hlist_node *h)
{
	return h->pprev == &h->next;
}

/*
 * Move a list from one list head to another. Fixup the pprev
 * reference of the first entry if it exists.
 */
static inline void rtl_hlist_move_list(struct rtl_hlist_head *old,
									   struct rtl_hlist_head *new)
{
	new->first = old->first;
	if (new->first)
		new->first->pprev = &new->first;
	old->first = NULL;
}

#define rtl_hlist_entry(ptr, type, member) rtl_container_of(ptr,type,member)

#define rtl_hlist_for_each(pos, head) \
	for (pos = (head)->first; pos ; pos = pos->next)

#define rtl_hlist_for_each_safe(pos, n, head) \
	for (pos = (head)->first; pos && ({ n = pos->next; 1; }); \
			pos = n)

#define rtl_hlist_entry_safe(ptr, type, member) \
	({ typeof(ptr) ____ptr = (ptr); \
	 ____ptr ? rtl_hlist_entry(____ptr, type, member) : NULL; \
	 })

/**
 * rtl_hlist_for_each_entry	- iterate over list of given type
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your list.
 * @member:	the name of the rtl_hlist_node within the struct.
 */
#define rtl_hlist_for_each_entry(pos, head, member)				\
	for (pos = rtl_hlist_entry_safe((head)->first, typeof(*(pos)), member);\
			pos;							\
			pos = rtl_hlist_entry_safe((pos)->member.next, typeof(*(pos)), member))

/**
 * rtl_hlist_for_each_entry_continue - iterate over a hlist continuing after current point
 * @pos:	the type * to use as a loop cursor.
 * @member:	the name of the rtl_hlist_node within the struct.
 */
#define rtl_hlist_for_each_entry_continue(pos, member)			\
	for (pos = rtl_hlist_entry_safe((pos)->member.next, typeof(*(pos)), member);\
			pos;							\
			pos = rtl_hlist_entry_safe((pos)->member.next, typeof(*(pos)), member))

/**
 * rtl_hlist_for_each_entry_from - iterate over a hlist continuing from current point
 * @pos:	the type * to use as a loop cursor.
 * @member:	the name of the rtl_hlist_node within the struct.
 */
#define rtl_hlist_for_each_entry_from(pos, member)				\
	for (; pos;							\
			pos = rtl_hlist_entry_safe((pos)->member.next, typeof(*(pos)), member))

/**
 * rtl_hlist_for_each_entry_safe - iterate over list of given type safe against removal of list entry
 * @pos:	the type * to use as a loop cursor.
 * @n:		another &struct rtl_hlist_node to use as temporary storage
 * @head:	the head for your list.
 * @member:	the name of the rtl_hlist_node within the struct.
 */
#define rtl_hlist_for_each_entry_safe(pos, n, head, member) 		\
	for (pos = rtl_hlist_entry_safe((head)->first, typeof(*pos), member);\
			pos && ({ n = pos->member.next; 1; });			\
			pos = rtl_hlist_entry_safe(n, typeof(*pos), member))

#endif /* _RTL_LIST_H_ */
