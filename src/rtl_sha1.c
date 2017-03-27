#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rtl_sha1.h"

#define ROTLEFT(a, b) ((a << b) | (a >> (32 - b)))

static void sha1_transform(rtl_sha1_ctx *ctx, const uint8_t data[])
{
	uint32_t a, b, c, d, e, i, j, t, m[80];

	for (i = 0, j = 0; i < 16; ++i, j += 4)
		m[i] = (data[j] << 24) + (data[j + 1] << 16) + (data[j + 2] << 8) + (data[j + 3]);
	for ( ; i < 80; ++i) {
		m[i] = (m[i - 3] ^ m[i - 8] ^ m[i - 14] ^ m[i - 16]);
		m[i] = (m[i] << 1) | (m[i] >> 31);
	}

	a = ctx->state[0];
	b = ctx->state[1];
	c = ctx->state[2];
	d = ctx->state[3];
	e = ctx->state[4];

	for (i = 0; i < 20; ++i) {
		t = ROTLEFT(a, 5) + ((b & c) ^ (~b & d)) + e + ctx->k[0] + m[i];
		e = d;
		d = c;
		c = ROTLEFT(b, 30);
		b = a;
		a = t;
	}
	for ( ; i < 40; ++i) {
		t = ROTLEFT(a, 5) + (b ^ c ^ d) + e + ctx->k[1] + m[i];
		e = d;
		d = c;
		c = ROTLEFT(b, 30);
		b = a;
		a = t;
	}
	for ( ; i < 60; ++i) {
		t = ROTLEFT(a, 5) + ((b & c) ^ (b & d) ^ (c & d))  + e + ctx->k[2] + m[i];
		e = d;
		d = c;
		c = ROTLEFT(b, 30);
		b = a;
		a = t;
	}
	for ( ; i < 80; ++i) {
		t = ROTLEFT(a, 5) + (b ^ c ^ d) + e + ctx->k[3] + m[i];
		e = d;
		d = c;
		c = ROTLEFT(b, 30);
		b = a;
		a = t;
	}

	ctx->state[0] += a;
	ctx->state[1] += b;
	ctx->state[2] += c;
	ctx->state[3] += d;
	ctx->state[4] += e;
}

void rtl_sha1_init(rtl_sha1_ctx *ctx)
{
	ctx->datalen = 0;
	ctx->bitlen = 0;
	ctx->state[0] = 0x67452301;
	ctx->state[1] = 0xEFCDAB89;
	ctx->state[2] = 0x98BADCFE;
	ctx->state[3] = 0x10325476;
	ctx->state[4] = 0xc3d2e1f0;
	ctx->k[0] = 0x5a827999;
	ctx->k[1] = 0x6ed9eba1;
	ctx->k[2] = 0x8f1bbcdc;
	ctx->k[3] = 0xca62c1d6;
}

void rtl_sha1_update(rtl_sha1_ctx *ctx, const uint8_t data[], size_t len)
{
	size_t i;

	for (i = 0; i < len; ++i) {
		ctx->data[ctx->datalen] = data[i];
		ctx->datalen++;
		if (ctx->datalen == 64) {
			sha1_transform(ctx, ctx->data);
			ctx->bitlen += 512;
			ctx->datalen = 0;
		}
	}
}

void rtl_sha1_final(rtl_sha1_ctx *ctx, uint8_t hash[])
{
	uint32_t i;

	i = ctx->datalen;

	/* Pad whatever data is left in the buffer. */
	if (ctx->datalen < 56) {
		ctx->data[i++] = 0x80;
		while (i < 56)
			ctx->data[i++] = 0x00;
	} else {
		ctx->data[i++] = 0x80;
		while (i < 64)
			ctx->data[i++] = 0x00;
		sha1_transform(ctx, ctx->data);
		memset(ctx->data, 0, 56);
	}

	/* Append to the padding the total message's length in bits and transform. */
	ctx->bitlen += ctx->datalen * 8;
	ctx->data[63] = ctx->bitlen;
	ctx->data[62] = ctx->bitlen >> 8;
	ctx->data[61] = ctx->bitlen >> 16;
	ctx->data[60] = ctx->bitlen >> 24;
	ctx->data[59] = ctx->bitlen >> 32;
	ctx->data[58] = ctx->bitlen >> 40;
	ctx->data[57] = ctx->bitlen >> 48;
	ctx->data[56] = ctx->bitlen >> 56;
	sha1_transform(ctx, ctx->data);

	/* Since this implementation uses little endian byte ordering and MD uses big endian,
	 * reverse all the bytes when copying the final state to the output hash.
	 */
	for (i = 0; i < 4; ++i) {
		hash[i]      = (ctx->state[0] >> (24 - i * 8)) & 0x000000ff;
		hash[i + 4]  = (ctx->state[1] >> (24 - i * 8)) & 0x000000ff;
		hash[i + 8]  = (ctx->state[2] >> (24 - i * 8)) & 0x000000ff;
		hash[i + 12] = (ctx->state[3] >> (24 - i * 8)) & 0x000000ff;
		hash[i + 16] = (ctx->state[4] >> (24 - i * 8)) & 0x000000ff;
	}
}

int rtl_sha1_string(const char *str, char *result, size_t size)
{
	rtl_sha1_ctx ctx;
	unsigned char sha1[RTL_SHA1_BLOCK_SIZE];

	if (size < RTL_SHA1_BLOCK_SIZE * 2 + 1)
		return -1;

	rtl_sha1_init(&ctx);
	rtl_sha1_update(&ctx, (uint8_t *)str, strlen(str));
	rtl_sha1_final(&ctx, sha1);

	int i;
	for (i = 0; i < RTL_SHA1_BLOCK_SIZE; i++)
		sprintf(result + i * 2, "%02x", sha1[i]);

	return 0;
}

/*
 * @filename: file name
 * @result: char array
 * @size: the size of result
 */
int rtl_sha1_file(const char *filename, char *result, size_t size)
{
	rtl_sha1_ctx ctx;
	unsigned char buffer[4096];
	unsigned char sha1[RTL_SHA1_BLOCK_SIZE];

	if (size < RTL_SHA1_BLOCK_SIZE * 2 + 1)
		return -1;

	FILE *fp;
	if ((fp = fopen(filename, "rb")) == NULL)
		return -1;

	rtl_sha1_init(&ctx);

	int len;
	while ((len = fread(buffer, 1, sizeof(buffer), fp)) > 0)
		rtl_sha1_update(&ctx, buffer, len);

	rtl_sha1_final(&ctx, sha1);

	int i;
	for (i = 0; i < RTL_SHA1_BLOCK_SIZE; i++)
		sprintf(result + i * 2, "%02x", sha1[i]);

	fclose(fp);

	return 0;
}
