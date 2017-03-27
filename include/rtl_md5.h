/*
 * This is an OpenSSL-compatible implementation of the RSA Data Security, Inc.
 * MD5 Message-Digest Algorithm (RFC 1321).
 *
 * Homepage:
 * http://openwall.info/wiki/people/solar/software/public-domain-source-code/md5
 *
 * Author:
 * Alexander Peslyak, better known as Solar Designer <solar at openwall.com>
 *
 * This software was written by Alexander Peslyak in 2001.  No copyright is
 * claimed, and the software is hereby placed in the public domain.
 * In case this attempt to disclaim copyright and place the software in the
 * public domain is deemed null and void, then the software is
 * Copyright (c) 2001 Alexander Peslyak and it is hereby released to the
 * general public under the following terms:
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted.
 *
 * There's ABSOLUTELY NO WARRANTY, express or implied.
 *
 * See md5.c for more information.
 */

#ifndef _RTL_MD5_H_
#define _RTL_MD5_H_

#include <stdint.h>

#define RTL_MD5_BLOCK_SIZE	16	/* MD5 outputs a 16 byte digest */

typedef struct {
	uint32_t lo, hi;
	uint32_t a, b, c, d;
	unsigned char buffer[64];
	uint32_t block[16];
} rtl_md5_ctx;

void rtl_md5_init(rtl_md5_ctx *ctx);
void rtl_md5_update(rtl_md5_ctx *ctx, const void *data, unsigned long size);
void rtl_md5_final(rtl_md5_ctx *ctx, unsigned char *result);
int rtl_md5_file(const char *filename, char *result, size_t size);

#endif /* _RTL_MD5_H_ */
