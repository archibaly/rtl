#ifndef _RTL_JSON_H_
#define _RTL_JSON_H_

#ifdef __cplusplus
extern "C" {
#endif

/* JSON Types: */
#define RTL_JSON_FALSE				0
#define RTL_JSON_TRUE				1
#define RTL_JSON_NULL				2
#define RTL_JSON_NUMBER				3
#define RTL_JSON_STRING				4
#define RTL_JSON_ARRAY				5
#define RTL_JSON_OBJECT				6

#define RTL_JSON_IS_REFERENCE		256
#define RTL_JSON_STRING_IS_CONST	512

/* the rtl_json_t structure: */
typedef struct json {
	struct json *next, *prev;	/* next/prev allow you to walk array/object
								   chains. alternatively, use get_array_size/
								   get_array_item/get_object_item */
	struct json *child;			/* an array or object item will have a child
								   pointer pointing to a chain of the items in
								   the array/object. */

	int type;					/* the type of the item, as above. */

	char *valuestring;			/* the item's string, if type==RTL_JSON_STRING */
	int valueint;				/* the item's number, if type==RTL_JSON_NUMBER */
	double valuedouble;			/* the item's number, if type==RTL_JSON_NUMBER */

	char *string;				/* the item's name string, if this item is the
								   child of, or is in the list of subitems of an object. */
} rtl_json_t;


/* supply a block of json, and this returns a rtl_json_t object you can interrogate.
 * call rtl_json_delete when finished.
 */
extern rtl_json_t *rtl_json_parse(const char *value);
/* render a rtl_json_t entity to text for transfer/storage. free the char* when
 * finished.
 */
extern char *rtl_json_print(rtl_json_t *item);
/* render a rtl_json_t entity to text for transfer/storage without any
 * formatting. free the char* when finished.
 */
extern char *rtl_json_print_unformatted(rtl_json_t *item);
/* render a rtl_json_t entity to text using a buffered strategy. prebuffer is a
 * guess at the final size. guessing well reduces reallocation. fmt=0 gives
 * unformatted, =1 gives formatted
 */
extern char *rtl_json_print_buffered(rtl_json_t *item, int prebuffer, int fmt);
/* delete a rtl_json_t entity and all subentities. */
extern void rtl_json_delete(rtl_json_t *c);

/* returns the number of items in an array (or object). */
extern int rtl_json_get_array_size(rtl_json_t *array);
/* retrieve item number "item" from array "array". returns null if unsuccessful. */
extern rtl_json_t *rtl_json_get_array_item(rtl_json_t *array, int item);
/* get item "string" from object. case insensitive. */
extern rtl_json_t *rtl_json_get_object_item(rtl_json_t *object, const char *string);

/* for analysing failed parses. this returns a pointer to the parse error.
 * you'll probably need to look a few chars back to make sense of it. defined
 * when rtl_json_parse() returns 0. 0 when rtl_json_parse() succeeds.
 */
extern const char *rtl_json_get_error_ptr(void);

/* these calls create a rtl_json_t item of the appropriate type. */
extern rtl_json_t *rtl_json_create_null(void);
extern rtl_json_t *rtl_json_create_true(void);
extern rtl_json_t *rtl_json_create_false(void);
extern rtl_json_t *rtl_json_create_bool(int b);
extern rtl_json_t *rtl_json_create_number(double num);
extern rtl_json_t *rtl_json_create_string(const char *string);
extern rtl_json_t *rtl_json_create_array(void);
extern rtl_json_t *rtl_json_create_object(void);

/* these utilities create an array of count items. */
extern rtl_json_t *rtl_json_create_int_array(const int *numbers, int count);
extern rtl_json_t *rtl_json_create_float_array(const float *numbers, int count);
extern rtl_json_t *rtl_json_create_double_array(const double *numbers, int count);
extern rtl_json_t *rtl_json_create_string_array(const char **strings, int count);

/* append item to the specified array/object. */
extern void rtl_json_add_item_to_array(rtl_json_t *array, rtl_json_t *item);
extern void rtl_json_add_item_to_object(rtl_json_t *object, const char *string,
								  rtl_json_t *item);
/* use this when string is definitely const (i.e. a literal, or as good as), and will definitely survive the rtl_json_t object */
extern void rtl_json_add_item_to_objectcs(rtl_json_t *object, const char *string, rtl_json_t *item);
/* append reference to item to the specified array/object. use this when you want to add an existing rtl_json_t to a new rtl_json_t, but don't want to corrupt your existing rtl_json_t. */
extern void rtl_json_add_item_reference_to_array(rtl_json_t *array, rtl_json_t *item);
extern void rtl_json_add_item_reference_to_object(rtl_json_t *object,
										   const char *string,
										   rtl_json_t *item);

/* remove/detatch items from arrays/objects. */
extern rtl_json_t *rtl_json_detach_item_from_array(rtl_json_t *array, int which);
extern void rtl_json_delete_item_from_array(rtl_json_t *array, int which);
extern rtl_json_t *rtl_json_detach_item_from_object(rtl_json_t *object,
										 const char *string);
extern void rtl_json_delete_item_from_object(rtl_json_t *object, const char *string);

/* update array items. */
extern void rtl_json_insert_item_in_array(rtl_json_t *array, int which, rtl_json_t *newitem);	/* shifts pre-existing items to the right. */
extern void rtl_json_replace_item_in_array(rtl_json_t *array, int which,
									 rtl_json_t *newitem);
extern void rtl_json_replace_item_ino_bject(rtl_json_t *object, const char *string,
									  rtl_json_t *newitem);

/* duplicate a rtl_json_t item */
extern rtl_json_t *rtl_json_duplicate(rtl_json_t *item, int recurse);
/* duplicate will create a new, identical rtl_json_t item to the one you pass, in new memory that will
need to be released. with recurse!=0, it will duplicate any children connected to the item.
the item->next and ->prev pointers are always zero on return from duplicate. */

/* parsewithopts allows you to require (and check) that the json is null terminated, and to retrieve the pointer to the final byte parsed. */
extern rtl_json_t *rtl_json_parse_with_opts(const char *value,
								  const char **return_parse_end,
								  int require_null_terminated);

extern void rtl_json_minify(char *json);

/* macros for creating things quickly. */
#define rtl_json_add_null_to_object(object,name)		rtl_json_add_item_to_object(object, name, rtl_json_create_null())
#define rtl_json_add_true_to_object(object,name)		rtl_json_add_item_to_object(object, name, rtl_json_create_true())
#define rtl_json_add_false_to_object(object,name)		rtl_json_add_item_to_object(object, name, rtl_json_create_false())
#define rtl_json_add_bool_to_object(object,name,b)		rtl_json_add_item_to_object(object, name, rtl_json_create_bool(b))
#define rtl_json_add_number_to_object(object,name,n)	rtl_json_add_item_to_object(object, name, rtl_json_create_number(n))
#define rtl_json_add_string_to_object(object,name,s)	rtl_json_add_item_to_object(object, name, rtl_json_create_string(s))

/* when assigning an integer value, it needs to be propagated to valuedouble too. */
#define rtl_json_set_int_value(object,val)				((object)?(object)->valueint=(object)->valuedouble=(val):(val))
#define rtl_json_set_number_value(object,val)			((object)?(object)->valueint=(object)->valuedouble=(val):(val))

#ifdef __cplusplus
}
#endif
#endif	/* _RTL_JSON_H_ */
