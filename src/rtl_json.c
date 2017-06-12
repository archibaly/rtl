#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <stdlib.h>
#include <float.h>
#include <limits.h>
#include <ctype.h>
#include <locale.h>

#include "rtl_json.h"

typedef struct {
	const unsigned char *json;
	size_t position;
} error;

static error global_error = { NULL, 0 };

const char *rtl_json_get_error_ptr(void)
{
	return (const char *)(global_error.json + global_error.position);
}

/* Case insensitive string comparison, doesn't consider two NULL pointers equal though */
static int case_insensitive_strcmp(const unsigned char *string1,
								   const unsigned char *string2)
{
	if ((string1 == NULL) || (string2 == NULL)) {
		return 1;
	}

	if (string1 == string2) {
		return 0;
	}

	for (; tolower(*string1) == tolower(*string2); (void)string1++, string2++) {
		if (*string1 == '\0') {
			return 0;
		}
	}

	return tolower(*string1) - tolower(*string2);
}

typedef struct internal_hooks {
	void *(*allocate) (size_t size);
	void (*deallocate) (void *pointer);
	void *(*reallocate) (void *pointer, size_t size);
} internal_hooks;

static internal_hooks global_hooks = { malloc, free, realloc };

static unsigned char *rtl_json_strdup(const unsigned char *string,
									  const internal_hooks * const hooks)
{
	size_t length = 0;
	unsigned char *copy = NULL;

	if (string == NULL) {
		return NULL;
	}

	length = strlen((const char *)string) + sizeof("");
	if (!(copy = (unsigned char *)hooks->allocate(length))) {
		return NULL;
	}
	memcpy(copy, string, length);

	return copy;
}

void rtl_json_init_hooks(rtl_json_hooks_t * hooks)
{
	if (hooks == NULL) {
		/* Reset hooks */
		global_hooks.allocate = malloc;
		global_hooks.deallocate = free;
		global_hooks.reallocate = realloc;
		return;
	}

	global_hooks.allocate = malloc;
	if (hooks->malloc_fn != NULL) {
		global_hooks.allocate = hooks->malloc_fn;
	}

	global_hooks.deallocate = free;
	if (hooks->free_fn != NULL) {
		global_hooks.deallocate = hooks->free_fn;
	}

	/* use realloc only if both free and malloc are used */
	global_hooks.reallocate = NULL;
	if ((global_hooks.allocate == malloc) && (global_hooks.deallocate == free)) {
		global_hooks.reallocate = realloc;
	}
}

/* Internal constructor. */
static rtl_json_t *rtl_json_new_item(const internal_hooks * const hooks)
{
	rtl_json_t *node = (rtl_json_t *) hooks->allocate(sizeof(rtl_json_t));

	if (node) {
		memset(node, '\0', sizeof(rtl_json_t));
	}

	return node;
}

/* Delete a rtl_json_t structure. */
void rtl_json_delete(rtl_json_t * item)
{
	rtl_json_t *next = NULL;

	while (item != NULL) {
		next = item->next;
		if (!(item->type & RTL_JSON_IS_REFERENCE) && (item->child != NULL)) {
			rtl_json_delete(item->child);
		}
		if (!(item->type & RTL_JSON_IS_REFERENCE)
			&& (item->valuestring != NULL)) {
			global_hooks.deallocate(item->valuestring);
		}
		if (!(item->type & RTL_JSON_STRING_IS_CONST) && (item->string != NULL)) {
			global_hooks.deallocate(item->string);
		}
		global_hooks.deallocate(item);
		item = next;
	}
}

/* get the decimal point character of the current locale */
static unsigned char get_decimal_point(void)
{
	struct lconv *lconv = localeconv();

	return (unsigned char)lconv->decimal_point[0];
}

typedef struct {
	const unsigned char *content;
	size_t length;
	size_t offset;
	/* How deeply nested (in arrays/objects) is the input at the current offset. */
	size_t depth;
	internal_hooks hooks;
} parse_buffer;

/* check if the given size is left to read in a given parse buffer (starting with 1) */
#define can_read(buffer, size) ((buffer != NULL) && (((buffer)->offset + size) <= (buffer)->length))
#define cannot_read(buffer, size) (!can_read(buffer, size))
/* check if the buffer can be accessed at the given index (starting with 0) */
#define can_access_at_index(buffer, index) ((buffer != NULL) && (((buffer)->offset + index) < (buffer)->length))
#define cannot_access_at_index(buffer, index) (!can_access_at_index(buffer, index))
/* get a pointer to the buffer at the position */
#define buffer_at_offset(buffer) ((buffer)->content + (buffer)->offset)

/* Parse the input text to generate a number, and populate the result into item. */
static int parse_number(rtl_json_t * const item,
						parse_buffer * const input_buffer)
{
	double number = 0;
	unsigned char *after_end = NULL;
	unsigned char number_c_string[64];
	unsigned char decimal_point = get_decimal_point();
	size_t i = 0;

	if ((input_buffer == NULL) || (input_buffer->content == NULL)) {
		return false;
	}

	/* copy the number into a temporary buffer and replace '.' with the decimal point
	 * of the current locale (for strtod)
	 * This also takes care of '\0' not necessarily being available for marking the end of the input */
	for (i = 0; (i < (sizeof(number_c_string) - 1))
		 && can_access_at_index(input_buffer, i); i++) {
		switch (buffer_at_offset(input_buffer)[i]) {
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
		case '+':
		case '-':
		case 'e':
		case 'E':
			number_c_string[i] = buffer_at_offset(input_buffer)[i];
			break;

		case '.':
			number_c_string[i] = decimal_point;
			break;

		default:
			goto loop_end;
		}
	}
  loop_end:
	number_c_string[i] = '\0';

	number = strtod((const char *)number_c_string, (char **)&after_end);
	if (number_c_string == after_end) {
		return false;			/* parse_error */
	}

	item->valuedouble = number;

	/* use saturation in case of overflow */
	if (number >= INT_MAX) {
		item->valueint = INT_MAX;
	} else if (number <= INT_MIN) {
		item->valueint = INT_MIN;
	} else {
		item->valueint = (int)number;
	}

	item->type = RTL_JSON_NUMBER;

	input_buffer->offset += (size_t) (after_end - number_c_string);
	return true;
}

/* don't ask me, but the original rtl_json_SetnumberValue returns an integer or double */
double rtl_json_set_number_helper(rtl_json_t * object, double number)
{
	if (number >= INT_MAX) {
		object->valueint = INT_MAX;
	} else if (number <= INT_MIN) {
		object->valueint = INT_MIN;
	} else {
		object->valueint = (int)number;
	}

	return object->valuedouble = number;
}

typedef struct {
	unsigned char *buffer;
	size_t length;
	size_t offset;
	size_t depth;				/* current nesting depth (for formatted printing) */
	int noalloc;
	int format;					/* is this print a formatted print */
	internal_hooks hooks;
} printbuffer;

/* realloc printbuffer if necessary to have at least "needed" bytes more */
static unsigned char *ensure(printbuffer * const p, size_t needed)
{
	unsigned char *newbuffer = NULL;
	size_t newsize = 0;

	if ((p == NULL) || (p->buffer == NULL)) {
		return NULL;
	}

	if ((p->length > 0) && (p->offset >= p->length)) {
		/* make sure that offset is valid */
		return NULL;
	}

	if (needed > INT_MAX) {
		/* sizes bigger than INT_MAX are currently not supported */
		return NULL;
	}

	needed += p->offset + 1;
	if (needed <= p->length) {
		return p->buffer + p->offset;
	}

	if (p->noalloc) {
		return NULL;
	}

	/* calculate new buffer size */
	if (needed > (INT_MAX / 2)) {
		/* overflow of int, use INT_MAX if possible */
		if (needed <= INT_MAX) {
			newsize = INT_MAX;
		} else {
			return NULL;
		}
	} else {
		newsize = needed * 2;
	}

	if (p->hooks.reallocate != NULL) {
		/* reallocate with realloc if available */
		newbuffer = (unsigned char *)p->hooks.reallocate(p->buffer, newsize);
	} else {
		/* otherwise reallocate manually */
		newbuffer = (unsigned char *)p->hooks.allocate(newsize);
		if (!newbuffer) {
			p->hooks.deallocate(p->buffer);
			p->length = 0;
			p->buffer = NULL;

			return NULL;
		}
		if (newbuffer) {
			memcpy(newbuffer, p->buffer, p->offset + 1);
		}
		p->hooks.deallocate(p->buffer);
	}
	p->length = newsize;
	p->buffer = newbuffer;

	return newbuffer + p->offset;
}

/* calculate the new length of the string in a printbuffer and update the offset */
static void update_offset(printbuffer * const buffer)
{
	const unsigned char *buffer_pointer = NULL;

	if ((buffer == NULL) || (buffer->buffer == NULL)) {
		return;
	}
	buffer_pointer = buffer->buffer + buffer->offset;

	buffer->offset += strlen((const char *)buffer_pointer);
}

/* Render the number nicely from the given item into a string. */
static int print_number(const rtl_json_t * const item,
						printbuffer * const output_buffer)
{
	unsigned char *output_pointer = NULL;
	double d = item->valuedouble;
	int length = 0;
	size_t i = 0;
	unsigned char number_buffer[26];	/* temporary buffer to print the number into */
	unsigned char decimal_point = get_decimal_point();
	double test;

	if (output_buffer == NULL) {
		return false;
	}

	/* This checks for NaN and Infinity */
	if ((d * 0) != 0) {
		length = sprintf((char *)number_buffer, "null");
	} else {
		/* Try 15 decimal places of precision to avoid nonsignificant nonzero digits */
		length = sprintf((char *)number_buffer, "%1.15g", d);

		/* Check whether the original double can be recovered */
		if ((sscanf((char *)number_buffer, "%lg", &test) != 1)
			|| ((double)test != d)) {
			/* If not, print with 17 decimal places of precision */
			length = sprintf((char *)number_buffer, "%1.17g", d);
		}
	}

	/* sprintf failed or buffer overrun occured */
	if ((length < 0) || (length > (int)(sizeof(number_buffer) - 1))) {
		return false;
	}

	/* reserve appropriate space in the output */
	output_pointer = ensure(output_buffer, (size_t) length);
	if (output_pointer == NULL) {
		return false;
	}

	/* copy the printed number to the output and replace locale
	 * dependent decimal point with '.' */
	for (i = 0; i < ((size_t) length); i++) {
		if (number_buffer[i] == decimal_point) {
			output_pointer[i] = '.';
			continue;
		}

		output_pointer[i] = number_buffer[i];
	}
	output_pointer[i] = '\0';

	output_buffer->offset += (size_t) length;

	return true;
}

/* parse 4 digit hexadecimal number */
static unsigned parse_hex4(const unsigned char *const input)
{
	unsigned int h = 0;
	size_t i = 0;

	for (i = 0; i < 4; i++) {
		/* parse digit */
		if ((input[i] >= '0') && (input[i] <= '9')) {
			h += (unsigned int)input[i] - '0';
		} else if ((input[i] >= 'A') && (input[i] <= 'F')) {
			h += (unsigned int)10 + input[i] - 'A';
		} else if ((input[i] >= 'a') && (input[i] <= 'f')) {
			h += (unsigned int)10 + input[i] - 'a';
		} else {				/* invalid */

			return 0;
		}

		if (i < 3) {
			/* shift left to make place for the next nibble */
			h = h << 4;
		}
	}

	return h;
}

/* converts a UTF-16 literal to UTF-8
 * A literal can be one or two sequences of the form \uXXXX */
static unsigned char utf16_literal_to_utf8(const unsigned char *const
										   input_pointer,
										   const unsigned char *const input_end,
										   unsigned char **output_pointer)
{
	long unsigned int codepoint = 0;
	unsigned int first_code = 0;
	const unsigned char *first_sequence = input_pointer;
	unsigned char utf8_length = 0;
	unsigned char utf8_position = 0;
	unsigned char sequence_length = 0;
	unsigned char first_byte_mark = 0;

	if ((input_end - first_sequence) < 6) {
		/* input ends unexpectedly */
		goto fail;
	}

	/* get the first utf16 sequence */
	first_code = parse_hex4(first_sequence + 2);

	/* check that the code is valid */
	if (((first_code >= 0xDC00) && (first_code <= 0xDFFF))) {
		goto fail;
	}

	/* UTF16 surrogate pair */
	if ((first_code >= 0xD800) && (first_code <= 0xDBFF)) {
		const unsigned char *second_sequence = first_sequence + 6;
		unsigned int second_code = 0;

		sequence_length = 12;	/* \uXXXX\uXXXX */

		if ((input_end - second_sequence) < 6) {
			/* input ends unexpectedly */
			goto fail;
		}

		if ((second_sequence[0] != '\\') || (second_sequence[1] != 'u')) {
			/* missing second half of the surrogate pair */
			goto fail;
		}

		/* get the second utf16 sequence */
		second_code = parse_hex4(second_sequence + 2);
		/* check that the code is valid */
		if ((second_code < 0xDC00) || (second_code > 0xDFFF)) {
			/* invalid second half of the surrogate pair */
			goto fail;
		}

		/* calculate the unicode codepoint from the surrogate pair */
		codepoint =
			0x10000 + (((first_code & 0x3FF) << 10) | (second_code & 0x3FF));
	} else {
		sequence_length = 6;	/* \uXXXX */
		codepoint = first_code;
	}

	/* encode as UTF-8
	 * takes at maximum 4 bytes to encode:
	 * 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx */
	if (codepoint < 0x80) {
		/* normal ascii, encoding 0xxxxxxx */
		utf8_length = 1;
	} else if (codepoint < 0x800) {
		/* two bytes, encoding 110xxxxx 10xxxxxx */
		utf8_length = 2;
		first_byte_mark = 0xC0;	/* 11000000 */
	} else if (codepoint < 0x10000) {
		/* three bytes, encoding 1110xxxx 10xxxxxx 10xxxxxx */
		utf8_length = 3;
		first_byte_mark = 0xE0;	/* 11100000 */
	} else if (codepoint <= 0x10FFFF) {
		/* four bytes, encoding 1110xxxx 10xxxxxx 10xxxxxx 10xxxxxx */
		utf8_length = 4;
		first_byte_mark = 0xF0;	/* 11110000 */
	} else {
		/* invalid unicode codepoint */
		goto fail;
	}

	/* encode as utf8 */
	for (utf8_position = (unsigned char)(utf8_length - 1); utf8_position > 0;
		 utf8_position--) {
		/* 10xxxxxx */
		(*output_pointer)[utf8_position] =
			(unsigned char)((codepoint | 0x80) & 0xBF);
		codepoint >>= 6;
	}
	/* encode first byte */
	if (utf8_length > 1) {
		(*output_pointer)[0] =
			(unsigned char)((codepoint | first_byte_mark) & 0xFF);
	} else {
		(*output_pointer)[0] = (unsigned char)(codepoint & 0x7F);
	}

	*output_pointer += utf8_length;

	return sequence_length;

  fail:
	return 0;
}

/* Parse the input text into an unescaped cinput, and populate item. */
static int parse_string(rtl_json_t * const item,
						parse_buffer * const input_buffer)
{
	const unsigned char *input_pointer = buffer_at_offset(input_buffer) + 1;
	const unsigned char *input_end = buffer_at_offset(input_buffer) + 1;
	unsigned char *output_pointer = NULL;
	unsigned char *output = NULL;

	/* not a string */
	if (buffer_at_offset(input_buffer)[0] != '\"') {
		goto fail;
	}

	{
		/* calculate approximate size of the output (overestimate) */
		size_t allocation_length = 0;
		size_t skipped_bytes = 0;

		while (((size_t) (input_end - input_buffer->content) <
				input_buffer->length) && (*input_end != '\"')) {
			/* is escape sequence */
			if (input_end[0] == '\\') {
				if ((size_t) (input_end + 1 - input_buffer->content) >=
					input_buffer->length) {
					/* prevent buffer overflow when last input character is a backslash */
					goto fail;
				}
				skipped_bytes++;
				input_end++;
			}
			input_end++;
		}
		if (((size_t) (input_end - input_buffer->content) >=
			 input_buffer->length) || (*input_end != '\"')) {
			goto fail;			/* string ended unexpectedly */
		}

		/* This is at most how much we need for the output */
		allocation_length =
			(size_t) (input_end - buffer_at_offset(input_buffer)) -
			skipped_bytes;
		output =
			(unsigned char *)input_buffer->hooks.allocate(allocation_length +
														  sizeof(""));
		if (output == NULL) {
			goto fail;			/* allocation failure */
		}
	}

	output_pointer = output;
	/* loop through the string literal */
	while (input_pointer < input_end) {
		if (*input_pointer != '\\') {
			*output_pointer++ = *input_pointer++;
		}
		/* escape sequence */
		else {
			unsigned char sequence_length = 2;

			if ((input_end - input_pointer) < 1) {
				goto fail;
			}

			switch (input_pointer[1]) {
			case 'b':
				*output_pointer++ = '\b';
				break;
			case 'f':
				*output_pointer++ = '\f';
				break;
			case 'n':
				*output_pointer++ = '\n';
				break;
			case 'r':
				*output_pointer++ = '\r';
				break;
			case 't':
				*output_pointer++ = '\t';
				break;
			case '\"':
			case '\\':
			case '/':
				*output_pointer++ = input_pointer[1];
				break;

				/* UTF-16 literal */
			case 'u':
				sequence_length =
					utf16_literal_to_utf8(input_pointer, input_end,
										  &output_pointer);
				if (sequence_length == 0) {
					/* failed to convert UTF16-literal to UTF-8 */
					goto fail;
				}
				break;

			default:
				goto fail;
			}
			input_pointer += sequence_length;
		}
	}

	/* zero terminate the output */
	*output_pointer = '\0';

	item->type = RTL_JSON_STRING;
	item->valuestring = (char *)output;

	input_buffer->offset = (size_t) (input_end - input_buffer->content);
	input_buffer->offset++;

	return true;

  fail:
	if (output != NULL) {
		input_buffer->hooks.deallocate(output);
	}

	if (input_pointer != NULL) {
		input_buffer->offset = (size_t) (input_pointer - input_buffer->content);
	}

	return false;
}

/* Render the cstring provided to an escaped version that can be printed. */
static int print_string_ptr(const unsigned char *const input,
							printbuffer * const output_buffer)
{
	const unsigned char *input_pointer = NULL;
	unsigned char *output = NULL;
	unsigned char *output_pointer = NULL;
	size_t output_length = 0;

	/* numbers of additional characters needed for escaping */
	size_t escape_characters = 0;

	if (output_buffer == NULL) {
		return false;
	}

	/* empty string */
	if (input == NULL) {
		output = ensure(output_buffer, sizeof("\"\""));
		if (output == NULL) {
			return false;
		}
		strcpy((char *)output, "\"\"");

		return true;
	}

	/* set "flag" to 1 if something needs to be escaped */
	for (input_pointer = input; *input_pointer; input_pointer++) {
		switch (*input_pointer) {
		case '\"':
		case '\\':
		case '\b':
		case '\f':
		case '\n':
		case '\r':
		case '\t':
			/* one character escape sequence */
			escape_characters++;
			break;
		default:
			if (*input_pointer < 32) {
				/* UTF-16 escape sequence uXXXX */
				escape_characters += 5;
			}
			break;
		}
	}
	output_length = (size_t) (input_pointer - input) + escape_characters;

	output = ensure(output_buffer, output_length + sizeof("\"\""));
	if (output == NULL) {
		return false;
	}

	/* no characters have to be escaped */
	if (escape_characters == 0) {
		output[0] = '\"';
		memcpy(output + 1, input, output_length);
		output[output_length + 1] = '\"';
		output[output_length + 2] = '\0';

		return true;
	}

	output[0] = '\"';
	output_pointer = output + 1;
	/* copy the string */
	for (input_pointer = input; *input_pointer != '\0';
		 (void)input_pointer++, output_pointer++) {
		if ((*input_pointer > 31) && (*input_pointer != '\"')
			&& (*input_pointer != '\\')) {
			/* normal character, copy */
			*output_pointer = *input_pointer;
		} else {
			/* character needs to be escaped */
			*output_pointer++ = '\\';
			switch (*input_pointer) {
			case '\\':
				*output_pointer = '\\';
				break;
			case '\"':
				*output_pointer = '\"';
				break;
			case '\b':
				*output_pointer = 'b';
				break;
			case '\f':
				*output_pointer = 'f';
				break;
			case '\n':
				*output_pointer = 'n';
				break;
			case '\r':
				*output_pointer = 'r';
				break;
			case '\t':
				*output_pointer = 't';
				break;
			default:
				/* escape and print as unicode codepoint */
				sprintf((char *)output_pointer, "u%04x", *input_pointer);
				output_pointer += 4;
				break;
			}
		}
	}
	output[output_length + 1] = '\"';
	output[output_length + 2] = '\0';

	return true;
}

/* Invoke print_string_ptr (which is useful) on an item. */
static int print_string(const rtl_json_t * const item, printbuffer * const p)
{
	return print_string_ptr((unsigned char *)item->valuestring, p);
}

/* Predeclare these prototypes. */
static int parse_value(rtl_json_t * const item,
					   parse_buffer * const input_buffer);
static int print_value(const rtl_json_t * const item,
					   printbuffer * const output_buffer);
static int parse_array(rtl_json_t * const item,
					   parse_buffer * const input_buffer);
static int print_array(const rtl_json_t * const item,
					   printbuffer * const output_buffer);
static int parse_object(rtl_json_t * const item,
						parse_buffer * const input_buffer);
static int print_object(const rtl_json_t * const item,
						printbuffer * const output_buffer);

/* Utility to jump whitespace and cr/lf */
static parse_buffer *buffer_skip_whitespace(parse_buffer * const buffer)
{
	if ((buffer == NULL) || (buffer->content == NULL)) {
		return NULL;
	}

	while (can_access_at_index(buffer, 0)
		   && (buffer_at_offset(buffer)[0] <= 32)) {
		buffer->offset++;
	}

	if (buffer->offset == buffer->length) {
		buffer->offset--;
	}

	return buffer;
}

/* Parse an object - create a new root, and populate. */
rtl_json_t *rtl_json_parse_with_opts(const char *value,
									 const char **return_parse_end,
									 int require_null_terminated)
{
	parse_buffer buffer = { 0, 0, 0, 0, {0, 0, 0} };
	rtl_json_t *item = NULL;

	/* reset error position */
	global_error.json = NULL;
	global_error.position = 0;

	if (value == NULL) {
		goto fail;
	}

	buffer.content = (const unsigned char *)value;
	buffer.length = strlen((const char *)value) + sizeof("");
	buffer.offset = 0;
	buffer.hooks = global_hooks;

	item = rtl_json_new_item(&global_hooks);
	if (item == NULL) {			/* memory fail */
		goto fail;
	}

	if (!parse_value(item, buffer_skip_whitespace(&buffer))) {
		/* parse failure. ep is set. */
		goto fail;
	}

	/* if we require null-terminated JSON without appended garbage, skip and then check for a null terminator */
	if (require_null_terminated) {
		buffer_skip_whitespace(&buffer);
		if ((buffer.offset >= buffer.length)
			|| buffer_at_offset(&buffer)[0] != '\0') {
			goto fail;
		}
	}
	if (return_parse_end) {
		*return_parse_end = (const char *)buffer_at_offset(&buffer);
	}

	return item;

  fail:
	if (item != NULL) {
		rtl_json_delete(item);
	}

	if (value != NULL) {
		error local_error;

		local_error.json = (const unsigned char *)value;
		local_error.position = 0;

		if (buffer.offset < buffer.length) {
			local_error.position = buffer.offset;
		} else if (buffer.length > 0) {
			local_error.position = buffer.length - 1;
		}

		if (return_parse_end != NULL) {
			*return_parse_end =
				(const char *)local_error.json + local_error.position;
		} else {
			global_error = local_error;
		}
	}

	return NULL;
}

/* Default options for rtl_json_Parse */
rtl_json_t *rtl_json_parse(const char *value)
{
	return rtl_json_parse_with_opts(value, 0, 0);
}

#define rtl_json_min(a, b) ((a < b) ? a : b)

static unsigned char *print(const rtl_json_t * const item, int format,
							const internal_hooks * const hooks)
{
	printbuffer buffer[1];
	unsigned char *printed = NULL;

	memset(buffer, 0, sizeof(buffer));

	/* create buffer */
	buffer->buffer = (unsigned char *)hooks->allocate(256);
	buffer->format = format;
	buffer->hooks = *hooks;
	if (buffer->buffer == NULL) {
		goto fail;
	}

	/* print the value */
	if (!print_value(item, buffer)) {
		goto fail;
	}
	update_offset(buffer);

	/* check if reallocate is available */
	if (hooks->reallocate != NULL) {
		printed =
			(unsigned char *)hooks->reallocate(buffer->buffer, buffer->length);
		buffer->buffer = NULL;
		if (printed == NULL) {
			goto fail;
		}
	} else {					/* otherwise copy the JSON over to a new buffer */

		printed = (unsigned char *)hooks->allocate(buffer->offset + 1);
		if (printed == NULL) {
			goto fail;
		}
		memcpy(printed, buffer->buffer,
			   rtl_json_min(buffer->length, buffer->offset + 1));
		printed[buffer->offset] = '\0';	/* just to be sure */

		/* free the buffer */
		hooks->deallocate(buffer->buffer);
	}

	return printed;

  fail:
	if (buffer->buffer != NULL) {
		hooks->deallocate(buffer->buffer);
	}

	if (printed != NULL) {
		hooks->deallocate(printed);
	}

	return NULL;
}

/* Render a rtl_json_t item/entity/structure to text. */
char *rtl_json_print(const rtl_json_t * item)
{
	return (char *)print(item, true, &global_hooks);
}

char *rtl_json_print_unformatted(const rtl_json_t * item)
{
	return (char *)print(item, false, &global_hooks);
}

char *rtl_json_print_buffered(const rtl_json_t * item, int prebuffer, int fmt)
{
	printbuffer p = { 0, 0, 0, 0, 0, 0, {0, 0, 0} };

	if (prebuffer < 0) {
		return NULL;
	}

	p.buffer = (unsigned char *)global_hooks.allocate((size_t) prebuffer);
	if (!p.buffer) {
		return NULL;
	}

	p.length = (size_t) prebuffer;
	p.offset = 0;
	p.noalloc = false;
	p.format = fmt;
	p.hooks = global_hooks;

	if (!print_value(item, &p)) {
		return NULL;
	}

	return (char *)p.buffer;
}

int rtl_json_print_preallocated(rtl_json_t * item, char *buf, const int len,
								const int fmt)
{
	printbuffer p = { 0, 0, 0, 0, 0, 0, {0, 0, 0} };

	if (len < 0) {
		return false;
	}

	p.buffer = (unsigned char *)buf;
	p.length = (size_t) len;
	p.offset = 0;
	p.noalloc = true;
	p.format = fmt;
	p.hooks = global_hooks;

	return print_value(item, &p);
}

/* Parser core - when encountering text, process appropriately. */
static int parse_value(rtl_json_t * const item,
					   parse_buffer * const input_buffer)
{
	if ((input_buffer == NULL) || (input_buffer->content == NULL)) {
		return false;			/* no input */
	}

	/* parse the different types of values */
	/* null */
	if (can_read(input_buffer, 4)
		&& (strncmp((const char *)buffer_at_offset(input_buffer), "null", 4) ==
			0)) {
		item->type = RTL_JSON_NULL;
		input_buffer->offset += 4;
		return true;
	}
	/* false */
	if (can_read(input_buffer, 5)
		&& (strncmp((const char *)buffer_at_offset(input_buffer), "false", 5) ==
			0)) {
		item->type = RTL_JSON_FALSE;
		input_buffer->offset += 5;
		return true;
	}
	/* true */
	if (can_read(input_buffer, 4)
		&& (strncmp((const char *)buffer_at_offset(input_buffer), "true", 4) ==
			0)) {
		item->type = RTL_JSON_TRUE;
		item->valueint = 1;
		input_buffer->offset += 4;
		return true;
	}
	/* string */
	if (can_access_at_index(input_buffer, 0)
		&& (buffer_at_offset(input_buffer)[0] == '\"')) {
		return parse_string(item, input_buffer);
	}
	/* number */
	if (can_access_at_index(input_buffer, 0)
		&& ((buffer_at_offset(input_buffer)[0] == '-')
			|| ((buffer_at_offset(input_buffer)[0] >= '0')
				&& (buffer_at_offset(input_buffer)[0] <= '9')))) {
		return parse_number(item, input_buffer);
	}
	/* array */
	if (can_access_at_index(input_buffer, 0)
		&& (buffer_at_offset(input_buffer)[0] == '[')) {
		return parse_array(item, input_buffer);
	}
	/* object */
	if (can_access_at_index(input_buffer, 0)
		&& (buffer_at_offset(input_buffer)[0] == '{')) {
		return parse_object(item, input_buffer);
	}

	return false;
}

/* Render a value to text. */
static int print_value(const rtl_json_t * const item,
					   printbuffer * const output_buffer)
{
	unsigned char *output = NULL;

	if ((item == NULL) || (output_buffer == NULL)) {
		return false;
	}

	switch ((item->type) & 0xFF) {
	case RTL_JSON_NULL:
		output = ensure(output_buffer, 5);
		if (output == NULL) {
			return false;
		}
		strcpy((char *)output, "null");
		return true;

	case RTL_JSON_FALSE:
		output = ensure(output_buffer, 6);
		if (output == NULL) {
			return false;
		}
		strcpy((char *)output, "false");
		return true;

	case RTL_JSON_TRUE:
		output = ensure(output_buffer, 5);
		if (output == NULL) {
			return false;
		}
		strcpy((char *)output, "true");
		return true;

	case RTL_JSON_NUMBER:
		return print_number(item, output_buffer);

	case RTL_JSON_RAW:
		{
			size_t raw_length = 0;

			if (item->valuestring == NULL) {
				if (!output_buffer->noalloc) {
					output_buffer->hooks.deallocate(output_buffer->buffer);
				}
				return false;
			}

			raw_length = strlen(item->valuestring) + sizeof("");
			output = ensure(output_buffer, raw_length);
			if (output == NULL) {
				return false;
			}
			memcpy(output, item->valuestring, raw_length);
			return true;
		}

	case RTL_JSON_STRING:
		return print_string(item, output_buffer);

	case RTL_JSON_ARRAY:
		return print_array(item, output_buffer);

	case RTL_JSON_OBJECT:
		return print_object(item, output_buffer);

	default:
		return false;
	}
}

/* Build an array from input text. */
static int parse_array(rtl_json_t * const item,
					   parse_buffer * const input_buffer)
{
	rtl_json_t *head = NULL;	/* head of the linked list */
	rtl_json_t *current_item = NULL;

	if (input_buffer->depth >= RTL_JSON_NESTING_LIMIT) {
		return false;			/* to deeply nested */
	}
	input_buffer->depth++;

	if (buffer_at_offset(input_buffer)[0] != '[') {
		/* not an array */
		goto fail;
	}

	input_buffer->offset++;
	buffer_skip_whitespace(input_buffer);
	if (can_access_at_index(input_buffer, 0)
		&& (buffer_at_offset(input_buffer)[0] == ']')) {
		/* empty array */
		goto success;
	}

	/* check if we skipped to the end of the buffer */
	if (cannot_access_at_index(input_buffer, 0)) {
		input_buffer->offset--;
		goto fail;
	}

	/* step back to character in front of the first element */
	input_buffer->offset--;
	/* loop through the comma separated array elements */
	do {
		/* allocate next item */
		rtl_json_t *new_item = rtl_json_new_item(&(input_buffer->hooks));

		if (new_item == NULL) {
			goto fail;			/* allocation failure */
		}

		/* attach next item to list */
		if (head == NULL) {
			/* start the linked list */
			current_item = head = new_item;
		} else {
			/* add to the end and advance */
			current_item->next = new_item;
			new_item->prev = current_item;
			current_item = new_item;
		}

		/* parse next value */
		input_buffer->offset++;
		buffer_skip_whitespace(input_buffer);
		if (!parse_value(current_item, input_buffer)) {
			goto fail;			/* failed to parse value */
		}
		buffer_skip_whitespace(input_buffer);
	}
	while (can_access_at_index(input_buffer, 0)
		   && (buffer_at_offset(input_buffer)[0] == ','));

	if (cannot_access_at_index(input_buffer, 0)
		|| buffer_at_offset(input_buffer)[0] != ']') {
		goto fail;				/* expected end of array */
	}

  success:
	input_buffer->depth--;

	item->type = RTL_JSON_ARRAY;
	item->child = head;

	input_buffer->offset++;

	return true;

  fail:
	if (head != NULL) {
		rtl_json_delete(head);
	}

	return false;
}

/* Render an array to text */
static int print_array(const rtl_json_t * const item,
					   printbuffer * const output_buffer)
{
	unsigned char *output_pointer = NULL;
	size_t length = 0;
	rtl_json_t *current_element = item->child;

	if (output_buffer == NULL) {
		return false;
	}

	/* Compose the output array. */
	/* opening square bracket */
	output_pointer = ensure(output_buffer, 1);
	if (output_pointer == NULL) {
		return false;
	}

	*output_pointer = '[';
	output_buffer->offset++;
	output_buffer->depth++;

	while (current_element != NULL) {
		if (!print_value(current_element, output_buffer)) {
			return false;
		}
		update_offset(output_buffer);
		if (current_element->next) {
			length = (size_t) (output_buffer->format ? 2 : 1);
			output_pointer = ensure(output_buffer, length + 1);
			if (output_pointer == NULL) {
				return false;
			}
			*output_pointer++ = ',';
			if (output_buffer->format) {
				*output_pointer++ = ' ';
			}
			*output_pointer = '\0';
			output_buffer->offset += length;
		}
		current_element = current_element->next;
	}

	output_pointer = ensure(output_buffer, 2);
	if (output_pointer == NULL) {
		return false;
	}
	*output_pointer++ = ']';
	*output_pointer = '\0';
	output_buffer->depth--;

	return true;
}

/* Build an object from the text. */
static int parse_object(rtl_json_t * const item,
						parse_buffer * const input_buffer)
{
	rtl_json_t *head = NULL;	/* linked list head */
	rtl_json_t *current_item = NULL;

	if (input_buffer->depth >= RTL_JSON_NESTING_LIMIT) {
		return false;			/* to deeply nested */
	}
	input_buffer->depth++;

	if (cannot_access_at_index(input_buffer, 0)
		|| (buffer_at_offset(input_buffer)[0] != '{')) {
		goto fail;				/* not an object */
	}

	input_buffer->offset++;
	buffer_skip_whitespace(input_buffer);
	if (can_access_at_index(input_buffer, 0)
		&& (buffer_at_offset(input_buffer)[0] == '}')) {
		goto success;			/* empty object */
	}

	/* check if we skipped to the end of the buffer */
	if (cannot_access_at_index(input_buffer, 0)) {
		input_buffer->offset--;
		goto fail;
	}

	/* step back to character in front of the first element */
	input_buffer->offset--;
	/* loop through the comma separated array elements */
	do {
		/* allocate next item */
		rtl_json_t *new_item = rtl_json_new_item(&(input_buffer->hooks));

		if (new_item == NULL) {
			goto fail;			/* allocation failure */
		}

		/* attach next item to list */
		if (head == NULL) {
			/* start the linked list */
			current_item = head = new_item;
		} else {
			/* add to the end and advance */
			current_item->next = new_item;
			new_item->prev = current_item;
			current_item = new_item;
		}

		/* parse the name of the child */
		input_buffer->offset++;
		buffer_skip_whitespace(input_buffer);
		if (!parse_string(current_item, input_buffer)) {
			goto fail;			/* faile to parse name */
		}
		buffer_skip_whitespace(input_buffer);

		/* swap valuestring and string, because we parsed the name */
		current_item->string = current_item->valuestring;
		current_item->valuestring = NULL;

		if (cannot_access_at_index(input_buffer, 0)
			|| (buffer_at_offset(input_buffer)[0] != ':')) {
			goto fail;			/* invalid object */
		}

		/* parse the value */
		input_buffer->offset++;
		buffer_skip_whitespace(input_buffer);
		if (!parse_value(current_item, input_buffer)) {
			goto fail;			/* failed to parse value */
		}
		buffer_skip_whitespace(input_buffer);
	}
	while (can_access_at_index(input_buffer, 0)
		   && (buffer_at_offset(input_buffer)[0] == ','));

	if (cannot_access_at_index(input_buffer, 0)
		|| (buffer_at_offset(input_buffer)[0] != '}')) {
		goto fail;				/* expected end of object */
	}

  success:
	input_buffer->depth--;

	item->type = RTL_JSON_OBJECT;
	item->child = head;

	input_buffer->offset++;
	return true;

  fail:
	if (head != NULL) {
		rtl_json_delete(head);
	}

	return false;
}

/* Render an object to text. */
static int print_object(const rtl_json_t * const item,
						printbuffer * const output_buffer)
{
	unsigned char *output_pointer = NULL;
	size_t length = 0;
	rtl_json_t *current_item = item->child;

	if (output_buffer == NULL) {
		return false;
	}

	/* Compose the output: */
	length = (size_t) (output_buffer->format ? 2 : 1);	/* fmt: {\n */
	output_pointer = ensure(output_buffer, length + 1);
	if (output_pointer == NULL) {
		return false;
	}

	*output_pointer++ = '{';
	output_buffer->depth++;
	if (output_buffer->format) {
		*output_pointer++ = '\n';
	}
	output_buffer->offset += length;

	while (current_item) {
		if (output_buffer->format) {
			size_t i;

			output_pointer = ensure(output_buffer, output_buffer->depth);
			if (output_pointer == NULL) {
				return false;
			}
			for (i = 0; i < output_buffer->depth; i++) {
				*output_pointer++ = '\t';
			}
			output_buffer->offset += output_buffer->depth;
		}

		/* print key */
		if (!print_string_ptr
			((unsigned char *)current_item->string, output_buffer)) {
			return false;
		}
		update_offset(output_buffer);

		length = (size_t) (output_buffer->format ? 2 : 1);
		output_pointer = ensure(output_buffer, length);
		if (output_pointer == NULL) {
			return false;
		}
		*output_pointer++ = ':';
		if (output_buffer->format) {
			*output_pointer++ = '\t';
		}
		output_buffer->offset += length;

		/* print value */
		if (!print_value(current_item, output_buffer)) {
			return false;
		}
		update_offset(output_buffer);

		/* print comma if not last */
		length =
			(size_t) ((output_buffer->format ? 1 : 0) +
					  (current_item->next ? 1 : 0));
		output_pointer = ensure(output_buffer, length + 1);
		if (output_pointer == NULL) {
			return false;
		}
		if (current_item->next) {
			*output_pointer++ = ',';
		}

		if (output_buffer->format) {
			*output_pointer++ = '\n';
		}
		*output_pointer = '\0';
		output_buffer->offset += length;

		current_item = current_item->next;
	}

	output_pointer =
		ensure(output_buffer,
			   output_buffer->format ? (output_buffer->depth + 1) : 2);
	if (output_pointer == NULL) {
		return false;
	}
	if (output_buffer->format) {
		size_t i;

		for (i = 0; i < (output_buffer->depth - 1); i++) {
			*output_pointer++ = '\t';
		}
	}
	*output_pointer++ = '}';
	*output_pointer = '\0';
	output_buffer->depth--;

	return true;
}

/* Get array size/item / object item. */
int rtl_json_get_array_size(const rtl_json_t * array)
{
	rtl_json_t *c = array->child;
	size_t i = 0;

	while (c) {
		i++;
		c = c->next;
	}

	/* FIXME: Can overflow here. Cannot be fixed without breaking the API */

	return (int)i;
}

static rtl_json_t *get_array_item(const rtl_json_t * array, size_t index)
{
	rtl_json_t *current_child = NULL;

	if (array == NULL) {
		return NULL;
	}

	current_child = array->child;
	while ((current_child != NULL) && (index > 0)) {
		index--;
		current_child = current_child->next;
	}

	return current_child;
}

rtl_json_t *rtl_json_get_array_item(const rtl_json_t * array, int index)
{
	if (index < 0) {
		return NULL;
	}

	return get_array_item(array, (size_t) index);
}

static rtl_json_t *get_object_item(const rtl_json_t * const object,
								   const char *const name,
								   const int case_sensitive)
{
	rtl_json_t *current_element = NULL;

	if ((object == NULL) || (name == NULL)) {
		return NULL;
	}

	current_element = object->child;
	if (case_sensitive) {
		while ((current_element != NULL)
			   && (strcmp(name, current_element->string) != 0)) {
			current_element = current_element->next;
		}
	} else {
		while ((current_element != NULL)
			   &&
			   (case_insensitive_strcmp
				((const unsigned char *)name,
				 (const unsigned char *)(current_element->string)) != 0)) {
			current_element = current_element->next;
		}
	}

	return current_element;
}

rtl_json_t *rtl_json_get_object_item(const rtl_json_t * const object,
									 const char *const string)
{
	return get_object_item(object, string, false);
}

rtl_json_t *rtl_json_get_object_itemcase_sensitive(const rtl_json_t *
												   const object,
												   const char *const string)
{
	return get_object_item(object, string, true);
}

int rtl_json_has_object_item(const rtl_json_t * object, const char *string)
{
	return rtl_json_get_object_item(object, string) ? 1 : 0;
}

/* Utility for array list handling. */
static void suffix_object(rtl_json_t * prev, rtl_json_t * item)
{
	prev->next = item;
	item->prev = prev;
}

/* Utility for handling references. */
static rtl_json_t *create_reference(const rtl_json_t * item,
									const internal_hooks * const hooks)
{
	rtl_json_t *ref = rtl_json_new_item(hooks);

	if (!ref) {
		return NULL;
	}
	memcpy(ref, item, sizeof(rtl_json_t));
	ref->string = NULL;
	ref->type |= RTL_JSON_IS_REFERENCE;
	ref->next = ref->prev = NULL;
	return ref;
}

/* Add item to array/object. */
void rtl_json_add_item_to_array(rtl_json_t * array, rtl_json_t * item)
{
	rtl_json_t *child = NULL;

	if ((item == NULL) || (array == NULL)) {
		return;
	}

	child = array->child;

	if (child == NULL) {
		/* list is empty, start new one */
		array->child = item;
	} else {
		/* append to the end */
		while (child->next) {
			child = child->next;
		}
		suffix_object(child, item);
	}
}

void rtl_json_add_item_to_object(rtl_json_t * object, const char *string,
								 rtl_json_t * item)
{
	/* call rtl_json_add_item_to_objectCS for code reuse */
	rtl_json_add_item_to_object_cs(object,
								   (char *)
								   rtl_json_strdup((const unsigned char *)
												   string, &global_hooks),
								   item);
	/* remove RTL_JSON_STRING_IS_CONST flag */
	item->type &= ~RTL_JSON_STRING_IS_CONST;
}

/* Add an item to an object with constant string as key */
void rtl_json_add_item_to_object_cs(rtl_json_t * object, const char *string,
									rtl_json_t * item)
{
	if (!item) {
		return;
	}
	if (!(item->type & RTL_JSON_STRING_IS_CONST) && item->string) {
		global_hooks.deallocate(item->string);
	}
	item->string = (char *)string;
	item->type |= RTL_JSON_STRING_IS_CONST;
	rtl_json_add_item_to_array(object, item);
}

void rtl_json_add_item_reference_to_array(rtl_json_t * array, rtl_json_t * item)
{
	rtl_json_add_item_to_array(array, create_reference(item, &global_hooks));
}

void rtl_json_add_item_reference_to_object(rtl_json_t * object,
										   const char *string,
										   rtl_json_t * item)
{
	rtl_json_add_item_to_object(object, string,
								create_reference(item, &global_hooks));
}

rtl_json_t *rtl_json_detach_item_via_pointer(rtl_json_t * parent,
											 rtl_json_t * const item)
{
	if ((parent == NULL) || (item == NULL)) {
		return NULL;
	}

	if (item->prev != NULL) {
		/* not the first element */
		item->prev->next = item->next;
	}
	if (item->next != NULL) {
		/* not the last element */
		item->next->prev = item->prev;
	}

	if (item == parent->child) {
		/* first element */
		parent->child = item->next;
	}
	/* make sure the detached item doesn't point anywhere anymore */
	item->prev = NULL;
	item->next = NULL;

	return item;
}

rtl_json_t *rtl_json_detach_item_from_array(rtl_json_t * array, int which)
{
	if (which < 0) {
		return NULL;
	}

	return rtl_json_detach_item_via_pointer(array,
											get_array_item(array,
														   (size_t) which));
}

void rtl_json_delete_item_from_array(rtl_json_t * array, int which)
{
	rtl_json_delete(rtl_json_detach_item_from_array(array, which));
}

rtl_json_t *rtl_json_detach_item_from_object(rtl_json_t * object,
											 const char *string)
{
	rtl_json_t *to_detach = rtl_json_get_object_item(object, string);

	return rtl_json_detach_item_via_pointer(object, to_detach);
}

rtl_json_t *rtl_json_detach_item_from_object_case_sensitive(rtl_json_t * object,
															const char *string)
{
	rtl_json_t *to_detach =
		rtl_json_get_object_itemcase_sensitive(object, string);

	return rtl_json_detach_item_via_pointer(object, to_detach);
}

void rtl_json_delete_item_from_object(rtl_json_t * object, const char *string)
{
	rtl_json_delete(rtl_json_detach_item_from_object(object, string));
}

void rtl_json_delete_item_from_object_case_sensitive(rtl_json_t * object,
													 const char *string)
{
	rtl_json_delete(rtl_json_detach_item_from_object_case_sensitive
					(object, string));
}

/* Replace array/object items with new ones. */
void rtl_json_insert_item_in_array(rtl_json_t * array, int which,
								   rtl_json_t * newitem)
{
	rtl_json_t *after_inserted = NULL;

	if (which < 0) {
		return;
	}

	after_inserted = get_array_item(array, (size_t) which);
	if (after_inserted == NULL) {
		rtl_json_add_item_to_array(array, newitem);
		return;
	}

	newitem->next = after_inserted;
	newitem->prev = after_inserted->prev;
	after_inserted->prev = newitem;
	if (after_inserted == array->child) {
		array->child = newitem;
	} else {
		newitem->prev->next = newitem;
	}
}

int rtl_json_replace_item_via_pointer(rtl_json_t * const parent,
									  rtl_json_t * const item,
									  rtl_json_t * replacement)
{
	if ((parent == NULL) || (replacement == NULL)) {
		return false;
	}

	if (replacement == item) {
		return true;
	}

	replacement->next = item->next;
	replacement->prev = item->prev;

	if (replacement->next != NULL) {
		replacement->next->prev = replacement;
	}
	if (replacement->prev != NULL) {
		replacement->prev->next = replacement;
	}
	if (parent->child == item) {
		parent->child = replacement;
	}

	item->next = NULL;
	item->prev = NULL;
	rtl_json_delete(item);

	return true;
}

void rtl_json_replace_item_inarray(rtl_json_t * array, int which,
								   rtl_json_t * newitem)
{
	if (which < 0) {
		return;
	}

	rtl_json_replace_item_via_pointer(array,
									  get_array_item(array, (size_t) which),
									  newitem);
}

static int replace_item_in_object(rtl_json_t * object, const char *string,
								  rtl_json_t * replacement, int case_sensitive)
{
	if (replacement == NULL) {
		return false;
	}

	/* replace the name in the replacement */
	if (!(replacement->type & RTL_JSON_STRING_IS_CONST)
		&& (replacement->string != NULL)) {
		rtl_json_free(replacement->string);
	}
	replacement->string =
		(char *)rtl_json_strdup((const unsigned char *)string, &global_hooks);
	replacement->type &= ~RTL_JSON_STRING_IS_CONST;

	rtl_json_replace_item_via_pointer(object,
									  get_object_item(object, string,
													  case_sensitive),
									  replacement);

	return true;
}

void rtl_json_replace_item_inobject(rtl_json_t * object, const char *string,
									rtl_json_t * newitem)
{
	replace_item_in_object(object, string, newitem, false);
}

void rtl_json_replace_item_in_object_case_sensitive(rtl_json_t * object,
													const char *string,
													rtl_json_t * newitem)
{
	replace_item_in_object(object, string, newitem, true);
}

/* create_ basic types: */
rtl_json_t *rtl_json_create_null(void)
{
	rtl_json_t *item = rtl_json_new_item(&global_hooks);

	if (item) {
		item->type = RTL_JSON_NULL;
	}

	return item;
}

rtl_json_t *rtl_json_create_true(void)
{
	rtl_json_t *item = rtl_json_new_item(&global_hooks);

	if (item) {
		item->type = RTL_JSON_TRUE;
	}

	return item;
}

rtl_json_t *rtl_json_create_false(void)
{
	rtl_json_t *item = rtl_json_new_item(&global_hooks);

	if (item) {
		item->type = RTL_JSON_FALSE;
	}

	return item;
}

rtl_json_t *rtl_json_create_bool(int b)
{
	rtl_json_t *item = rtl_json_new_item(&global_hooks);

	if (item) {
		item->type = b ? RTL_JSON_TRUE : RTL_JSON_FALSE;
	}

	return item;
}

rtl_json_t *rtl_json_create_number(double num)
{
	rtl_json_t *item = rtl_json_new_item(&global_hooks);

	if (item) {
		item->type = RTL_JSON_NUMBER;
		item->valuedouble = num;

		/* use saturation in case of overflow */
		if (num >= INT_MAX) {
			item->valueint = INT_MAX;
		} else if (num <= INT_MIN) {
			item->valueint = INT_MIN;
		} else {
			item->valueint = (int)num;
		}
	}

	return item;
}

rtl_json_t *rtl_json_create_string(const char *string)
{
	rtl_json_t *item = rtl_json_new_item(&global_hooks);

	if (item) {
		item->type = RTL_JSON_STRING;
		item->valuestring =
			(char *)rtl_json_strdup((const unsigned char *)string,
									&global_hooks);
		if (!item->valuestring) {
			rtl_json_delete(item);
			return NULL;
		}
	}

	return item;
}

rtl_json_t *rtl_json_create_raw(const char *raw)
{
	rtl_json_t *item = rtl_json_new_item(&global_hooks);

	if (item) {
		item->type = RTL_JSON_RAW;
		item->valuestring =
			(char *)rtl_json_strdup((const unsigned char *)raw, &global_hooks);
		if (!item->valuestring) {
			rtl_json_delete(item);
			return NULL;
		}
	}

	return item;
}

rtl_json_t *rtl_json_create_array(void)
{
	rtl_json_t *item = rtl_json_new_item(&global_hooks);

	if (item) {
		item->type = RTL_JSON_ARRAY;
	}

	return item;
}

rtl_json_t *rtl_json_create_object(void)
{
	rtl_json_t *item = rtl_json_new_item(&global_hooks);

	if (item) {
		item->type = RTL_JSON_OBJECT;
	}

	return item;
}

/* create_ arrays: */
rtl_json_t *rtl_json_create_int_array(const int *numbers, int count)
{
	size_t i = 0;
	rtl_json_t *n = NULL;
	rtl_json_t *p = NULL;
	rtl_json_t *a = NULL;

	if (count < 0) {
		return NULL;
	}

	a = rtl_json_create_array();
	for (i = 0; a && (i < (size_t) count); i++) {
		n = rtl_json_create_number(numbers[i]);
		if (!n) {
			rtl_json_delete(a);
			return NULL;
		}
		if (!i) {
			a->child = n;
		} else {
			suffix_object(p, n);
		}
		p = n;
	}

	return a;
}

rtl_json_t *rtl_json_create_float_array(const float *numbers, int count)
{
	size_t i = 0;
	rtl_json_t *n = NULL;
	rtl_json_t *p = NULL;
	rtl_json_t *a = NULL;

	if (count < 0) {
		return NULL;
	}

	a = rtl_json_create_array();

	for (i = 0; a && (i < (size_t) count); i++) {
		n = rtl_json_create_number((double)numbers[i]);
		if (!n) {
			rtl_json_delete(a);
			return NULL;
		}
		if (!i) {
			a->child = n;
		} else {
			suffix_object(p, n);
		}
		p = n;
	}

	return a;
}

rtl_json_t *rtl_json_create_double_array(const double *numbers, int count)
{
	size_t i = 0;
	rtl_json_t *n = NULL;
	rtl_json_t *p = NULL;
	rtl_json_t *a = NULL;

	if (count < 0) {
		return NULL;
	}

	a = rtl_json_create_array();

	for (i = 0; a && (i < (size_t) count); i++) {
		n = rtl_json_create_number(numbers[i]);
		if (!n) {
			rtl_json_delete(a);
			return NULL;
		}
		if (!i) {
			a->child = n;
		} else {
			suffix_object(p, n);
		}
		p = n;
	}

	return a;
}

rtl_json_t *rtl_json_create_string_array(const char **strings, int count)
{
	size_t i = 0;
	rtl_json_t *n = NULL;
	rtl_json_t *p = NULL;
	rtl_json_t *a = NULL;

	if (count < 0) {
		return NULL;
	}

	a = rtl_json_create_array();

	for (i = 0; a && (i < (size_t) count); i++) {
		n = rtl_json_create_string(strings[i]);
		if (!n) {
			rtl_json_delete(a);
			return NULL;
		}
		if (!i) {
			a->child = n;
		} else {
			suffix_object(p, n);
		}
		p = n;
	}

	return a;
}

/* Duplication */
rtl_json_t *rtl_json_duplicate(const rtl_json_t * item, int recurse)
{
	rtl_json_t *newitem = NULL;
	rtl_json_t *child = NULL;
	rtl_json_t *next = NULL;
	rtl_json_t *newchild = NULL;

	/* Bail on bad ptr */
	if (!item) {
		goto fail;
	}
	/* create_ new item */
	newitem = rtl_json_new_item(&global_hooks);
	if (!newitem) {
		goto fail;
	}
	/* Copy over all vars */
	newitem->type = item->type & (~RTL_JSON_IS_REFERENCE);
	newitem->valueint = item->valueint;
	newitem->valuedouble = item->valuedouble;
	if (item->valuestring) {
		newitem->valuestring =
			(char *)rtl_json_strdup((unsigned char *)item->valuestring,
									&global_hooks);
		if (!newitem->valuestring) {
			goto fail;
		}
	}
	if (item->string) {
		newitem->string =
			(item->type & RTL_JSON_STRING_IS_CONST) ? item->string : (char *)
			rtl_json_strdup((unsigned char *)item->string, &global_hooks);
		if (!newitem->string) {
			goto fail;
		}
	}
	/* If non-recursive, then we're done! */
	if (!recurse) {
		return newitem;
	}
	/* Walk the ->next chain for the child. */
	child = item->child;
	while (child != NULL) {
		/* Duplicate (with recurse) each item in the ->next chain */
		newchild = rtl_json_duplicate(child, true);
		if (!newchild) {
			goto fail;
		}
		if (next != NULL) {
			/* If newitem->child already set, then crosswire ->prev and ->next and move on */
			next->next = newchild;
			newchild->prev = next;
			next = newchild;
		} else {
			/* Set newitem->child and move to it */
			newitem->child = newchild;
			next = newchild;
		}
		child = child->next;
	}

	return newitem;

  fail:
	if (newitem != NULL) {
		rtl_json_delete(newitem);
	}

	return NULL;
}

void rtl_json_minify(char *json)
{
	unsigned char *into = (unsigned char *)json;

	while (*json) {
		if (*json == ' ') {
			json++;
		} else if (*json == '\t') {
			/* Whitespace characters. */
			json++;
		} else if (*json == '\r') {
			json++;
		} else if (*json == '\n') {
			json++;
		} else if ((*json == '/') && (json[1] == '/')) {
			/* double-slash comments, to end of line. */
			while (*json && (*json != '\n')) {
				json++;
			}
		} else if ((*json == '/') && (json[1] == '*')) {
			/* multiline comments. */
			while (*json && !((*json == '*') && (json[1] == '/'))) {
				json++;
			}
			json += 2;
		} else if (*json == '\"') {
			/* string literals, which are \" sensitive. */
			*into++ = (unsigned char)*json++;
			while (*json && (*json != '\"')) {
				if (*json == '\\') {
					*into++ = (unsigned char)*json++;
				}
				*into++ = (unsigned char)*json++;
			}
			*into++ = (unsigned char)*json++;
		} else {
			/* All other characters. */
			*into++ = (unsigned char)*json++;
		}
	}

	/* and null-terminate. */
	*into = '\0';
}

int rtl_json_is_invalid(const rtl_json_t * const item)
{
	if (item == NULL) {
		return false;
	}

	return (item->type & 0xFF) == RTL_JSON_INVALID;
}

int rtl_json_Is_False(const rtl_json_t * const item)
{
	if (item == NULL) {
		return false;
	}

	return (item->type & 0xFF) == RTL_JSON_FALSE;
}

int rtl_json_is_true(const rtl_json_t * const item)
{
	if (item == NULL) {
		return false;
	}

	return (item->type & 0xff) == RTL_JSON_TRUE;
}

int rtl_json_is_bool(const rtl_json_t * const item)
{
	if (item == NULL) {
		return false;
	}

	return (item->type & (RTL_JSON_TRUE | RTL_JSON_FALSE)) != 0;
}

int rtl_json_is_null(const rtl_json_t * const item)
{
	if (item == NULL) {
		return false;
	}

	return (item->type & 0xFF) == RTL_JSON_NULL;
}

int rtl_json_is_number(const rtl_json_t * const item)
{
	if (item == NULL) {
		return false;
	}

	return (item->type & 0xFF) == RTL_JSON_NUMBER;
}

int rtl_json_is_string(const rtl_json_t * const item)
{
	if (item == NULL) {
		return false;
	}

	return (item->type & 0xFF) == RTL_JSON_STRING;
}

int rtl_json_is_array(const rtl_json_t * const item)
{
	if (item == NULL) {
		return false;
	}

	return (item->type & 0xFF) == RTL_JSON_ARRAY;
}

int rtl_json_is_object(const rtl_json_t * const item)
{
	if (item == NULL) {
		return false;
	}

	return (item->type & 0xFF) == RTL_JSON_OBJECT;
}

int rtl_json_is_raw(const rtl_json_t * const item)
{
	if (item == NULL) {
		return false;
	}

	return (item->type & 0xFF) == RTL_JSON_RAW;
}

int rtl_json_compare(const rtl_json_t * const a, const rtl_json_t * const b,
					 const int case_sensitive)
{
	if ((a == NULL) || (b == NULL) || ((a->type & 0xFF) != (b->type & 0xFF))
		|| rtl_json_is_invalid(a)) {
		return false;
	}

	/* check if type is valid */
	switch (a->type & 0xFF) {
	case RTL_JSON_FALSE:
	case RTL_JSON_TRUE:
	case RTL_JSON_NULL:
	case RTL_JSON_NUMBER:
	case RTL_JSON_STRING:
	case RTL_JSON_RAW:
	case RTL_JSON_ARRAY:
	case RTL_JSON_OBJECT:
		break;

	default:
		return false;
	}

	/* identical objects are equal */
	if (a == b) {
		return true;
	}

	switch (a->type & 0xFF) {
		/* in these cases and equal type is enough */
	case RTL_JSON_FALSE:
	case RTL_JSON_TRUE:
	case RTL_JSON_NULL:
		return true;

	case RTL_JSON_NUMBER:
		if (a->valuedouble == b->valuedouble) {
			return true;
		}
		return false;

	case RTL_JSON_STRING:
	case RTL_JSON_RAW:
		if ((a->valuestring == NULL) || (b->valuestring == NULL)) {
			return false;
		}
		if (strcmp(a->valuestring, b->valuestring) == 0) {
			return true;
		}

		return false;

	case RTL_JSON_ARRAY:
		{
			rtl_json_t *a_element = a->child;
			rtl_json_t *b_element = b->child;

			for (; (a_element != NULL) && (b_element != NULL);) {
				if (!rtl_json_compare(a_element, b_element, case_sensitive)) {
					return false;
				}

				a_element = a_element->next;
				b_element = b_element->next;
			}

			return true;
		}

	case RTL_JSON_OBJECT:
		{
			rtl_json_t *a_element = NULL;

			rtl_json_array_for_each(a_element, a) {
				/* TODO This has O(n^2) runtime, which is horrible! */
				rtl_json_t *b_element =
					get_object_item(b, a_element->string, case_sensitive);
				if (b_element == NULL) {
					return false;
				}

				if (!rtl_json_compare(a_element, b_element, case_sensitive)) {
					return false;
				}
			}

			return true;
		}

	default:
		return false;
	}
}

void *rtl_json_malloc(size_t size)
{
	return global_hooks.allocate(size);
}

void rtl_json_free(void *object)
{
	global_hooks.deallocate(object);
}
