#ifndef _RTL_SHA256_H_
#define _RTL_SHA256_H_

#include <stdint.h>

#define RTL_SHA256_BLOCK_SIZE 32	/* SHA256 outputs a 32 byte digest */

typedef struct {
	uint8_t data[64];
	uint32_t datalen;
	unsigned long long bitlen;
	uint32_t state[8];
} rtl_sha256_ctx;

void rtl_sha256_init(rtl_sha256_ctx *ctx);
void rtl_sha256_update(rtl_sha256_ctx *ctx, const uint8_t data[], size_t len);
void rtl_sha256_final(rtl_sha256_ctx *ctx, uint8_t hash[]);
int rtl_sha256_string(const char *str, char *result, size_t size);
int rtl_sha256_file(const char *filename, char *result, size_t size);

#endif /* _RTL_SHA256_H_ */
