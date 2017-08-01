#ifndef _RTL_INI_H_
#define _RTL_INI_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rtl_dict.h"

/**
 * @brief    Configure a function to receive the error messages.
 * @param    errback  Function to call.
 *
 * By default, the error will be printed on stderr. If a null pointer is passed
 * as errback the error callback will be switched back to default.
 */
void rtl_ini_set_error_callback(int (*errback) (const char *, ...));

/**
 * @brief    Get number of sections in a rtl_dict_t
 * @param    d   Dictionary to examine
 * @return   int Number of sections found in rtl_dict_t
 *
 * This function returns the number of sections found in a rtl_dict_t.
 * The test to recognize sections is done on the string stored in the
 * rtl_dict_t: a section name is given as "section" whereas a key is
 * stored as "section:key", thus the test looks for entries that do not
 * contain a colon.
 *
 * This clearly fails in the case a section name contains a colon, but
 * this should simply be avoided.
 *
 * This function returns -1 in case of error.
 */
int rtl_ini_get_nsec(const rtl_dict_t *d);

/**
 * @brief    Get name for section n in a rtl_dict_t.
 * @param    d   Dictionary to examine
 * @param    n   Section number (from 0 to nsec-1).
 * @return   Pointer to char string
 *
 * This function locates the n-th section in a rtl_dict_t and returns
 * its name as a pointer to a string statically allocated inside the
 * rtl_dict_t. Do not free or modify the returned string!
 *
 * This function returns NULL in case of error.
 */
const char *rtl_ini_get_sec_name(const rtl_dict_t *d, int n);

/**
 * @brief    Save a rtl_dict_t to a loadable ini file
 * @param    d   Dictionary to dump
 * @param    f   Opened file pointer to dump to
 * @return   void
 *
 * This function dumps a given rtl_dict_t into a loadable ini file.
 * It is Ok to specify @c stderr or @c stdout as output files.
 */
void rtl_ini_dump_ini(const rtl_dict_t *d, FILE *f);

/**
 * @brief    Save a rtl_dict_t section to a loadable ini file
 * @param    d   Dictionary to dump
 * @param    s   Section name of rtl_dict_t to dump
 * @param    f   Opened file pointer to dump to
 * @return   void
 *
 * This function dumps a given section of a given rtl_dict_t into a loadable ini
 * file.  It is Ok to specify @c stderr or @c stdout as output files.
 */
void rtl_ini_dump_section_ini(const rtl_dict_t *d, const char *s, FILE *f);

/**
 * @brief    Dump a rtl_dict_t to an opened file pointer.
 * @param    d   Dictionary to dump.
 * @param    f   Opened file pointer to dump to.
 * @return   void
 *
 * This function prints out the contents of a rtl_dict_t, one element by
 * line, onto the provided file pointer. It is OK to specify @c stderr
 * or @c stdout as output files. This function is meant for debugging
 * purposes mostly.
 */
void rtl_ini_dump(const rtl_dict_t *d, FILE *f);

/**
 * @brief    Get the number of keys in a section of a rtl_dict_t.
 * @param    d   Dictionary to examine
 * @param    s   Section name of rtl_dict_t to examine
 * @return   Number of keys in section
 */
int rtl_ini_get_sec_nkeys(const rtl_dict_t *d, const char *s);

/**
 * @brief    Get the number of keys in a section of a rtl_dict_t.
 * @param    d    Dictionary to examine
 * @param    s    Section name of rtl_dict_t to examine
 * @param    keys Already allocated array to store the keys in
 * @return   The pointer passed as `keys` argument or NULL in case of error
 *
 * This function queries a rtl_dict_t and finds all keys in a given section.
 * The keys argument should be an array of pointers which size has been
 * determined by calling `rtl_ini_get_sec_nkeys` function prior to this one.
 *
 * Each pointer in the returned char pointer-to-pointer is pointing to
 * a string allocated in the rtl_dict_t; do not free or modify them.
 */
const char **rtl_ini_get_sec_keys(const rtl_dict_t *d, const char *s,
								  const char **keys);

/**
 * @brief    Get the string associated to a key
 * @param    d       Dictionary to search
 * @param    key     Key string to look for
 * @param    def     Default value to return if key not found.
 * @return   pointer to statically allocated character string
 *
 * This function queries a rtl_dict_t for a key. A key as read from an
 * ini file is given as "section:key". If the key cannot be found,
 * the pointer passed as 'def' is returned.
 * The returned char pointer is pointing to a string allocated in
 * the rtl_dict_t, do not free or modify it.
 */
const char *rtl_ini_get_string(const rtl_dict_t *d, const char *key,
							   const char *def);

/**
 * @brief    Get the string associated to a key, convert to an int
 * @param    d Dictionary to search
 * @param    key Key string to look for
 * @param    notfound Value to return in case of error
 * @return   integer
 *
 * This function queries a rtl_dict_t for a key. A key as read from an
 * ini file is given as "section:key". If the key cannot be found,
 * the notfound value is returned.
 *
 * Supported values for integers include the usual C notation
 * so decimal, octal (starting with 0) and hexadecimal (starting with 0x)
 * are supported. Examples:
 *
 * - "42"      ->  42
 * - "042"     ->  34 (octal -> decimal)
 * - "0x42"    ->  66 (hexa  -> decimal)
 *
 * Warning: the conversion may overflow in various ways. Conversion is
 * totally outsourced to strtol(), see the associated man page for overflow
 * handling.
 *
 * Credits: Thanks to A. Becker for suggesting strtol()
 */
int rtl_ini_get_int(const rtl_dict_t *d, const char *key, int notfound);

/**
 * @brief    Get the string associated to a key, convert to an long int
 * @param    d Dictionary to search
 * @param    key Key string to look for
 * @param    notfound Value to return in case of error
 * @return   integer
 *
 * This function queries a rtl_dict_t for a key. A key as read from an
 * ini file is given as "section:key". If the key cannot be found,
 * the notfound value is returned.
 *
 * Supported values for integers include the usual C notation
 * so decimal, octal (starting with 0) and hexadecimal (starting with 0x)
 * are supported. Examples:
 *
 * - "42"      ->  42
 * - "042"     ->  34 (octal -> decimal)
 * - "0x42"    ->  66 (hexa  -> decimal)
 *
 * Warning: the conversion may overflow in various ways. Conversion is
 * totally outsourced to strtol(), see the associated man page for overflow
 * handling.
 */
long int rtl_ini_get_longint(const rtl_dict_t *d, const char *key,
							 long int notfound);

/**
 * @brief    Get the string associated to a key, convert to a double
 * @param    d Dictionary to search
 * @param    key Key string to look for
 * @param    notfound Value to return in case of error
 * @return   double
 *
 * This function queries a rtl_dict_t for a key. A key as read from an
 * ini file is given as "section:key". If the key cannot be found,
 * the notfound value is returned.
 */
double rtl_ini_get_double(const rtl_dict_t *d, const char *key,
						  double notfound);

/**
 * @brief    Get the string associated to a key, convert to a boolean
 * @param    d Dictionary to search
 * @param    key Key string to look for
 * @param    notfound Value to return in case of error
 * @return   integer
 *
 * This function queries a rtl_dict_t for a key. A key as read from an
 * ini file is given as "section:key". If the key cannot be found,
 * the notfound value is returned.
 *
 * A true boolean is found if one of the following is matched:
 *
 * - A string starting with 'y'
 * - A string starting with 'Y'
 * - A string starting with 't'
 * - A string starting with 'T'
 * - A string starting with '1'
 *
 * A false boolean is found if one of the following is matched:
 *
 * - A string starting with 'n'
 * - A string starting with 'N'
 * - A string starting with 'f'
 * - A string starting with 'F'
 * - A string starting with '0'
 *
 * The notfound value returned if no boolean is identified, does not
 * necessarily have to be 0 or 1.
 */
int rtl_ini_get_boolean(const rtl_dict_t *d, const char *key, int notfound);

/**
 * @brief    Set an entry in a rtl_dict_t.
 * @param    ini     Dictionary to modify.
 * @param    entry   Entry to modify (entry name)
 * @param    val     New value to associate to the entry.
 * @return   int     0 if Ok, -1 otherwise.
 *
 * If the given entry can be found in the rtl_dict_t, it is modified to
 * contain the provided value. If it cannot be found, the entry is created.
 * It is Ok to set val to NULL.
 */
int rtl_ini_set(rtl_dict_t *ini, const char *entry, const char *val);

/**
 * @brief    Delete an entry in a rtl_dict_t
 * @param    ini     Dictionary to modify
 * @param    entry   Entry to delete (entry name)
 * @return   void
 *
 * If the given entry can be found, it is deleted from the rtl_dict_t.
 */
void rtl_ini_unset(rtl_dict_t *ini, const char *entry);

/**
 * @brief    Finds out if a given entry exists in a rtl_dict_t
 * @param    ini     Dictionary to search
 * @param    entry   Name of the entry to look for
 * @return   integer 1 if entry exists, 0 otherwise
 *
 * Finds out if a given entry exists in the rtl_dict_t. Since sections
 * are stored as keys with NULL associated values, this is the only way
 * of querying for the presence of sections in a rtl_dict_t.
 */
int rtl_ini_find_entry(const rtl_dict_t *ini, const char *entry);

/**
 * @brief    Parse an ini file and return an allocated rtl_dict_t object
 * @param    ininame Name of the ini file to read.
 * @return   Pointer to newly allocated rtl_dict_t
 *
 * This is the parser for ini files. This function is called, providing
 * the name of the file to be read. It returns a rtl_dict_t object that
 * should not be accessed directly, but through accessor functions
 * instead.
 *
 * The returned rtl_dict_t must be freed using rtl_ini_freedict().
 */
rtl_dict_t *rtl_ini_load(const char *ininame);

/**
 * @brief    Free all memory associated to an ini rtl_dict_t
 * @param    d Dictionary to free
 * @return   void
 *
 * Free all memory associated to an ini rtl_dict_t.
 * It is mandatory to call this function before the rtl_dict_t object
 * gets out of the current context.
 */
void rtl_ini_free_dict(rtl_dict_t *d);

#endif /* _RTL_INI_H_ */
