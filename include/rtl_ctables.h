/*
 * Ctables : Pretty table printer written in C
 * Author  : Enkitosh
 * git 	: http://github.com/enkitosh
 * Date	: 2013-Mar-04
 */

#ifndef _RTL_CTABLES_H_
#define _RTL_CTABLES_H_

#define RTL_MAX_OPS 4
#define RTL_MAX_BUF 50

/* COLORS */
#define RTL_BLACK	"\033[30m"
#define RTL_RED		"\033[31m"
#define RTL_GREEN	"\033[32m"
#define RTL_YELLOW	"\033[33m"
#define RTL_BLUE	"\033[34m"
#define RTL_MAGNETA	"\033[35m"
#define RTL_CYAN	"\033[36m"
#define RTL_WHITE	"\033[37m"
#define RTL_DEFAULT	"\033[39m"

/*
 * 0. STRICT - User adds dimensions, table handles the indexing
 *    FREELY - Data is added to table and user
 *    handles indexing
 * 1. Options - COLORFUL : make table sensitive to color and
 *    higlighting
 *    NOCOLOR  :     Prints B/W table
 * 2. Alignment - CENTER, LEFT, RIGHT
 *
 * 3. ENUMERATE - Display row/column numbers
 *    along table data. Pass NONE if
 *    this is not desired...
 */
enum {
	RTL_FREELY = 2,
	RTL_STRICT,
	RTL_COLORFUL,
	RTL_TRANSPARENT,
	RTL_NOCOLOR,
	RTL_CENTER,
	RTL_LEFT,
	RTL_RIGHT,
	RTL_ENUMERATE,
	RTL_NONE
};

typedef struct table_cell {
	char *str;
	char *color;
	int width;
	int cell_width;
	int max_cell_w;
} rtl_table_cell_t;

typedef struct table {
	int row_dimension;
	int col_dimension;
	int index_i;
	int index_j;
	int options[RTL_MAX_OPS];
	rtl_table_cell_t **info;
	int capacity;
} rtl_table_t;

/* Converts Int to String */
char *rtl_cnvrt_int(int x);

/* Converts hex value to string */
char *rtl_cnvrt_hex(int x);

/* Convert Memory Address to string */
char *rtl_cnvrt_ptr(void *ptr);

/* Operations */
rtl_table_t *rtl_initialize_table(int op[], int dim_i, int dim_j);

/* Add to STRICT table */
void rtl_add(rtl_table_t * table, char *str);

/* Add FREELY to table */
void rtl_add_freely(rtl_table_t * table, int row, int col, char *in_str);

/* Given a row and column, color the string at given index */
void rtl_color_me(rtl_table_t * table, int row, int col, char *color_c);

/* Finds a string in table and colors it */
void rtl_color_string(rtl_table_t * table, char *str_find, char *color_c);

/* Color all rows in a given column */
void rtl_color_row(rtl_table_t * table, int col, char *color_c);

/* Color all columns in a given row */
void rtl_color_columns(rtl_table_t * table, int row, char *color_c);

/* Prints out table */
void rtl_print(rtl_table_t * table);

/* Free's allocated table */
void rtl_free_table(rtl_table_t * table);

#endif /* _RTL_CTABLES_H_ */
