#ifndef _RTL_DICT_H_
#define _RTL_DICT_H_

#include <stdio.h>

/**
 * @brief    Dictionary object
 *
 * This object contains a list of string/string associations. Each
 * association is identified by a unique string key. Looking up values
 * in the rtl_dict_t is speeded up by the use of a (hopefully collision-free)
 * hash function.
 */
typedef struct rtl_dict {
	int n;					/** Number of entries in rtl_dict_t */
	ssize_t size;			/** Storage size */
	char **val;				/** List of string values */
	char **key;				/** List of string keys */
	unsigned *hash;			/** List of hash values for keys */
} rtl_dict_t;

/**
 * @brief    Compute the hash key for a string.
 * @param    key     Character string to use for key.
 * @return   1 unsigned int on at least 32 bits.
 *
 * This hash function has been taken from an Article in Dr Dobbs Journal.
 * This is normally a collision-free function, distributing keys evenly.
 * The key is stored anyway in the struct so that collision can be avoided
 * by comparing the key itself in last resort.
 */
unsigned rtl_dict_hash(const char *key);

/**
 * @brief    Create a new rtl_dict_t object.
 * @param    size    Optional initial size of the rtl_dict_t.
 * @return   1 newly allocated rtl_dict_t objet.
 *
 * This function allocates a new rtl_dict_t object of given size and returns
 * it. If you do not know in advance (roughly) the number of entries in the
 * rtl_dict_t, give size=0.
 */
rtl_dict_t *rtl_dict_new(size_t size);

/**
 * @brief    Delete a rtl_dict_t object
 * @param    d   rtl_dict_t object to deallocate.
 * @return   void
 *
 * Deallocate a rtl_dict_t object and all memory associated to it.
 */
void rtl_dict_del(rtl_dict_t *vd);

/**
 * @brief    Get a value from a rtl_dict_t.
 * @param    d       rtl_dict_t object to search.
 * @param    key     Key to look for in the rtl_dict_t.
 * @param    def     Default value to return if key not found.
 * @return   1 pointer to internally allocated character string.
 *
 * This function locates a key in a rtl_dict_t and returns a pointer to its
 * value, or the passed 'def' pointer if no such key can be found in
 * rtl_dict_t. The returned character pointer points to data internal to the
 * rtl_dict_t object, you should not try to free it or modify it.
 */
const char *rtl_dict_get(const rtl_dict_t *d, const char *key,
						 const char *def);

/**
 * @brief    Set a value in a rtl_dict_t.
 * @param    d       rtl_dict_t object to modify.
 * @param    key     Key to modify or add.
 * @param    val     Value to add.
 * @return   int     0 if Ok, anything else otherwise
 *
 * If the given key is found in the rtl_dict_t, the associated value is
 * replaced by the provided one. If the key cannot be found in the
 * rtl_dict_t, it is added to it.
 *
 * It is Ok to provide a NULL value for val, but NULL values for the rtl_dict_t
 * or the key are considered as errors: the function will return immediately
 * in such a case.
 *
 * Notice that if you dict_set a variable to NULL, a call to
 * dict_get will return a NULL value: the variable will be found, and
 * its value (NULL) is returned. In other words, setting the variable
 * content to NULL is equivalent to deleting the variable from the
 * rtl_dict_t. It is not possible (in this implementation) to have a key in
 * the rtl_dict_t without value.

 * This function returns non-zero in case of failure.
 */
int rtl_dict_set(rtl_dict_t *vd, const char *key, const char *val);

/**
 * @brief    Delete a key in a rtl_dict_t
 * @param    d       rtl_dict_t object to modify.
 * @param    key     Key to remove.
 * @return   void
 *
 * This function deletes a key in a rtl_dict_t. Nothing is done if the
 * key cannot be found.
 */
void rtl_dict_unset(rtl_dict_t *d, const char *key);

/**
 * @brief    Dump a rtl_dict_t to an opened file pointer.
 * @param    d   Dictionary to dump
 * @param    f   Opened file pointer.
 * @return   void
 *
 * Dumps a rtl_dict_t onto an opened file pointer. Key pairs are printed out
 * as @c [Key]=[Value], one per line. It is Ok to provide stdout or stderr as
 * output file pointers.
 */
void rtl_dict_dump(const rtl_dict_t *d, FILE *out);

#endif /* _RTL_DICT_H_ */
