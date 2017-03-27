#ifndef _RTL_BASE64_H_
#define _RTL_BASE64_H_

#include <stddef.h>
#include <stdint.h>

/* Returns the size of the output. If called with out = NULL, will just return
 * the size of what the output would have been (without a terminating NULL).
 */
size_t rtl_base64_encode(const uint8_t in[], uint8_t out[], size_t len, int newline_flag);

/* Returns the size of the output. If called with out = NULL, will just return
 * the size of what the output would have been (without a terminating NULL).
 */
size_t rtl_base64_decode(const uint8_t in[], uint8_t out[], size_t len);

#endif /* _RTL_BASE64_H_ */
