#ifndef _RTL_JSON_H_
#define _RTL_JSON_H_

#include <stddef.h>

/* rtl_json_t Types: */
#define RTL_JSON_INVALID	(0)
#define RTL_JSON_FALSE		(1 << 0)
#define RTL_JSON_TRUE		(1 << 1)
#define RTL_JSON_NULL		(1 << 2)
#define RTL_JSON_NUMBER		(1 << 3)
#define RTL_JSON_STRING		(1 << 4)
#define RTL_JSON_ARRAY		(1 << 5)
#define RTL_JSON_OBJECT		(1 << 6)
#define RTL_JSON_RAW		(1 << 7)	/* raw json */

#define RTL_JSON_IS_REFERENCE		256
#define RTL_JSON_STRING_IS_CONST	512

/* The rtl_json_t structure: */
typedef struct rtl_json {
	/* next/prev allow you to walk array/object chains.
	 * Alternatively, use get_array_size/get_array_item/get_object_item
	 */
	struct rtl_json *next;
	struct rtl_json *prev;
	/* An array or object item will have a child pointer pointing to a
	 * chain of the items in the array/object.
	 */
	struct rtl_json *child;

	/* The type of the item, as above. */
	int type;

	/* The item's string, if type==rtl_json_String  and type == rtl_json_Raw */
	char *valuestring;
	/* writing to valueint is DEPRECATED, use rtl_json_SetNumberValue instead */
	int valueint;
	/* The item's number, if type==rtl_json_Number */
	double valuedouble;

	/* The item's name string, if this item is the child of,
	 * or is in the list of subitems of an object.
	 */
	char *string;
} rtl_json_t;

typedef struct rtl_json_hooks {
	void *(*malloc_fn) (size_t sz);
	void (*free_fn) (void *ptr);
} rtl_json_hooks_t;

/* Limits how deeply nested arrays/objects can be before rtl_json_t rejects to
 * parse them. This is to prevent stack overflows.
 */
#ifndef RTL_JSON_NESTING_LIMIT
#define RTL_JSON_NESTING_LIMIT 1024
#endif

/* Supply malloc, realloc and free functions to rtl_json_t */
void rtl_json_init_hooks(rtl_json_hooks_t * hooks);

/* Memory Management: the caller is always responsible to free the results
 * from all variants of rtl_json_Parse (with rtl_json_Delete) and rtl_json_print
 * (with stdlib free, rtl_json_hooks.free_fn, or rtl_json_free as appropriate).
 * The exception is rtl_json_print_preallocated, where the caller has full
 * responsibility of the buffer.
 */
/* Supply a block of JSON, and this returns a rtl_json_t object you can
 * interrogate.
 */
rtl_json_t *rtl_json_parse(const char *value);

/* Render a rtl_json_t entity to text for transfer/storage. */
char *rtl_json_print(const rtl_json_t * item);

/* Render a rtl_json_t entity to text for transfer/storage without any formatting.
 */
char *rtl_json_print_unformatted(const rtl_json_t * item);

/* Render a rtl_json_t entity to text using a buffered strategy. prebuffer is a
 * guess at the final size. guessing well reduces reallocation. fmt=0 gives
 * unformatted, =1 gives formatted
 */
char *rtl_json_print_buffered(const rtl_json_t * item, int prebuffer, int fmt);

/* Render a rtl_json_t entity to text using a buffer already allocated in memory
 * with given length. Returns 1 on success and 0 on failure. */
/* NOTE: rtl_json_t is not always 100% accurate in estimating how much memory it
 * will use, so to be safe allocate 5 bytes more than you actually need
 */
int rtl_json_print_preallocated(rtl_json_t * item, char *buffer,
								const int length, const int format);
/* Delete a rtl_json_t entity and all subentities. */
void rtl_json_delete(rtl_json_t * c);

/* Returns the number of items in an array (or object). */
int rtl_json_get_array_size(const rtl_json_t * array);

/* Retrieve item number "item" from array "array". Returns NULL if unsuccessful.
 */
rtl_json_t *rtl_json_get_array_item(const rtl_json_t * array, int index);

/* Get item "string" from object. Case insensitive. */
rtl_json_t *rtl_json_get_object_item(const rtl_json_t * const object,
									 const char *const string);
rtl_json_t *rtl_json_get_object_item_case_sensitive(const rtl_json_t *
													const object,
													const char *const string);
int rtl_json_has_object_item(const rtl_json_t * object, const char *string);

/* For analysing failed parses. This returns a pointer to the parse error.
 * You'll probably need to look a few chars back to make sense of it.
 * Defined when rtl_json_parse() returns 0. 0 when rtl_json_parse() succeeds.
 */
const char *rtl_json_get_error_ptr(void);

/* These functions check the type of an item */
int rtl_json_is_invalid(const rtl_json_t * const item);
int rtl_json_is_false(const rtl_json_t * const item);
int rtl_json_is_true(const rtl_json_t * const item);
int rtl_json_is_bool(const rtl_json_t * const item);
int rtl_json_is_null(const rtl_json_t * const item);
int rtl_json_is_number(const rtl_json_t * const item);
int rtl_json_is_string(const rtl_json_t * const item);
int rtl_json_is_array(const rtl_json_t * const item);
int rtl_json_is_object(const rtl_json_t * const item);
int rtl_json_is_raw(const rtl_json_t * const item);

/* These calls create a rtl_json_t item of the appropriate type. */
rtl_json_t *rtl_json_create_null(void);
rtl_json_t *rtl_json_create_true(void);
rtl_json_t *rtl_json_create_false(void);

rtl_json_t *rtl_json_create_bool(int boolean);
rtl_json_t *rtl_json_create_number(double num);
rtl_json_t *rtl_json_create_string(const char *string);

/* raw json */
rtl_json_t *rtl_json_create_raw(const char *raw);
rtl_json_t *rtl_json_create_array(void);
rtl_json_t *rtl_json_create_object(void);

/* These utilities create an Array of count items. */
rtl_json_t *rtl_json_create_int_array(const int *numbers, int count);
rtl_json_t *rtl_json_create_float_array(const float *numbers, int count);
rtl_json_t *rtl_json_create_double_array(const double *numbers, int count);
rtl_json_t *rtl_json_create_string_array(const char **strings, int count);

/* Append item to the specified array/object. */
void rtl_json_add_item_to_array(rtl_json_t * array, rtl_json_t * item);
void rtl_json_add_item_to_object(rtl_json_t * object, const char *string,
								 rtl_json_t * item);

/* Use this when string is definitely const (i.e. a literal, or as good as),
 * and will definitely survive the rtl_json_t object.
 * WARNING: When this function was used, make sure to always check that
 * (item->type & rtl_json_string_is_const) is zero before writing to
 * `item->string`
 */
void rtl_json_add_item_to_object_cs(rtl_json_t * object, const char *string,
									rtl_json_t * item);

/* Append reference to item to the specified array/object. Use this when you
 * want to add an existing rtl_json_t to a new rtl_json_t, but don't want to corrupt
 * your existing rtl_json_t.
 */
void rtl_json_add_item_reference_toArray(rtl_json_t * array, rtl_json_t * item);
void rtl_json_add_item_reference_toObject(rtl_json_t * object,
										  const char *string,
										  rtl_json_t * item);

/* Remove/Detatch items from Arrays/Objects. */
rtl_json_t *rtl_json_detach_item_via_pointer(rtl_json_t * parent,
											 rtl_json_t * const item);
rtl_json_t *rtl_json_detach_item_from_array(rtl_json_t * array, int which);
void rtl_json_delete_item_from_array(rtl_json_t * array, int which);
rtl_json_t *rtl_json_detach_item_from_object(rtl_json_t * object,
											 const char *string);
rtl_json_t *rtl_json_detach_item_from_objectcasesensitive(rtl_json_t * object, const char
														  *string);
void rtl_json_delete_item_from_object(rtl_json_t * object, const char *string);
void rtl_json_delete_item_from_object_case_sensitive(rtl_json_t * object,
													 const char *string);

/* Update array items. */
/* Shifts pre-existing items to the right. */
void rtl_json_insert_item_in_array(rtl_json_t * array, int which,
								   rtl_json_t * newitem);
int rtl_json_replace_item_via_pointer(rtl_json_t * const parent,
									  rtl_json_t * const item,
									  rtl_json_t * replacement);
void rtl_json_replace_item_in_array(rtl_json_t * array, int which,
									rtl_json_t * newitem);
void rtl_json_replace_item_in_object(rtl_json_t * object, const char *string,
									 rtl_json_t * newitem);
void rtl_json_replace_item_in_object_case_sensitive(rtl_json_t * object,
													const char *string,
													rtl_json_t * newitem);

/* Duplicate a rtl_json_t item */
rtl_json_t *rtl_json_duplicate(const rtl_json_t * item, int recurse);

/* Duplicate will create a new, identical rtl_json_t item to the one you pass,
 * in new memory that will need to be released. With recurse!=0, it will
 * duplicate any children connected to the item.
 * The item->next and ->prev pointers are always zero on return from Duplicate.
 */
/* Recursively compare two rtl_json_t items for equality. If either a or b is NULL
 * or invalid, they will be considered unequal.
 * case_sensitive determines if object keys are treated case sensitive (1) or
 * case insensitive (0)
 */
int rtl_json_compare(const rtl_json_t * const a,
					 const rtl_json_t * const b, const int case_sensitive);

/* parse_with_opts allows you to require (and check) that the JSON is null
 * terminated, and to retrieve the pointer to the final byte parsed.
 */
/* If you supply a ptr in return_parse_end and parsing fails, then
 * return_parse_end will contain a pointer to the error. If not, then
 * rtl_json_get_error_ptr() does the job.
 */
rtl_json_t *rtl_json_parse_with_opts(const char *value,
									 const char **return_parse_end,
									 int require_null_terminated);

void rtl_json_minify(char *json);

/* Macros for creating things quickly. */
#define rtl_json_add_null_to_object(object, name)		rtl_json_add_item_to_object(object, name, rtl_json_create_null())
#define rtl_json_add_true_to_object(object, name)		rtl_json_add_item_to_object(object, name, rtl_json_create_true())
#define rtl_json_add_false_to_object(object, name)		rtl_json_add_item_to_object(object, name, rtl_json_create_false())
#define rtl_json_add_bool_to_object(object, name, b)	rtl_json_add_item_to_object(object, name, rtl_json_create_bool(b))
#define rtl_json_add_number_to_object(object, name, n)	rtl_json_add_item_to_object(object, name, rtl_json_create_number(n))
#define rtl_json_add_string_to_object(object, name, s)	rtl_json_add_item_to_object(object, name, rtl_json_create_string(s))
#define rtl_json_add_raw_to_object(object, name, s)		rtl_json_add_item_to_object(object, name, rtl_json_create_raw(s))

/* When assigning an integer value, it needs to be propagated to valuedouble too. */
#define rtl_json_set_int_value(object, number)	\
	((object) ? (object)->valueint = (object)->valuedouble = (number) : (number))
/* helper for the rtl_json_SetNumberValue macro */
double rtl_json_set_number_helper(rtl_json_t * object, double number);

#define rtl_json_set_number_value(object, number)	\
	((object != NULL) ? rtl_json_set_number_helper(object, (double)number) : (number))

/* Macro for iterating over an array or object */
#define rtl_json_array_for_each(element, array)	\
	for (element = (array != NULL) ? (array)->child : NULL; element != NULL; element = element->next)

/* malloc/free objects using the malloc/free functions that have been set with
 * rtl_json_init_hooks
 */
void *rtl_json_malloc(size_t size);
void rtl_json_free(void *object);

#endif /* _RTL_JSON_H_ */
