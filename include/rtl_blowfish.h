#ifndef _RTL_BLOWFISH_H_
#define _RTL_BLOWFISH_H_

#include <stddef.h>
#include <stdint.h>

#define RTL_BLOWFISH_BLOCK_SIZE	8	/* Blowfish operates on 8 bytes at a time */

typedef struct {
	uint32_t p[18];
	uint32_t s[4][256];
} rtl_blowfish_key_t;

void rtl_blowfish_key_setup(const uint8_t user_key[], rtl_blowfish_key_t *keystruct, size_t len);
void rtl_blowfish_encrypt(const uint8_t in[], uint8_t out[], const rtl_blowfish_key_t *keystruct);
void rtl_blowfish_decrypt(const uint8_t in[], uint8_t out[], const rtl_blowfish_key_t *keystruct);

#endif /* _RTL_BLOWFISH_H_ */
