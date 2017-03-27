#ifndef RTL_SHA1_H_
#define RTL_SHA1_H_

#include <stdint.h>

#define RTL_SHA1_BLOCK_SIZE	20	/* SHA1 outputs a 20 byte digest */

typedef struct {
	uint8_t data[64];
	uint32_t datalen;
	unsigned long long bitlen;
	uint32_t state[5];
	uint32_t k[4];
} rtl_sha1_ctx;

void rtl_sha1_init(rtl_sha1_ctx *ctx);
void rtl_sha1_update(rtl_sha1_ctx *ctx, const uint8_t data[], size_t len);
void rtl_sha1_final(rtl_sha1_ctx *ctx, uint8_t hash[]);
int rtl_sha1_string(const char *str, char *result, size_t size);
int rtl_sha1_file(const char *filename, char *result, size_t size);

#endif /* RTL_SHA1_H_ */
