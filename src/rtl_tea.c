#include <stdlib.h>
#include <string.h>

#include "rtl_tea.h"

/*
 * @v: the data need to be encrypted, must be 8 bytes long
 * @k: key, must be 16 bytes long
 */
static void encrypt(uint32_t *v, const uint32_t *k)
{
	uint32_t y = v[0], z = v[1], sum = 0, i;
	uint32_t delta = 0x9e3779b9;
	uint32_t a = k[0], b = k[1], c = k[2], d = k[3];

	for (i = 0; i < 32; i++) {
		sum += delta;
		y += ((z << 4) + a) ^ (z + sum) ^ ((z >> 5) + b);
		z += ((y << 4) + c) ^ (y + sum) ^ ((y >> 5) + d);
	}

	v[0] = y;
	v[1] = z;
}

/*
 * @v: the data need to be encrypted, must be 8 bytes long
 * @k: key, must be 16 bytes long
 */
static void decrypt(uint32_t *v, const uint32_t *k)
{
	uint32_t i;
	uint32_t y = v[0], z = v[1], sum = 0xc6ef3720;
	uint32_t delta = 0x9e3779b9;
	uint32_t a = k[0], b = k[1], c = k[2], d = k[3];

	for (i = 0; i < 32; i++) {
		z -= ((y << 4) + c) ^ (y + sum) ^ ((y >> 5) + d);
		y -= ((z << 4) + a) ^ (z + sum) ^ ((z >> 5) + b);
		sum -= delta;
	}

	v[0] = y;
	v[1] = z;
}

/*
 * @clear_text: the original plain text
 * @text_len: length of clear_text or cipher_text
 * @key: key, must be 16 bytes long
 * @return: cipher_text and must be freed outside this function
 */
uint8_t *rtl_tea_encrypt(const uint8_t *clear_text, int *text_len, const uint8_t *key)
{
	int i;

	/* PKCS7 padding */
	uint8_t padding_len = 8 - *text_len % 8;

	int len = *text_len + padding_len;
	uint8_t *cipher_text = malloc(len);
	if (!cipher_text)
		return NULL;

	memcpy(cipher_text, clear_text, *text_len);
	for (i = 0; i < padding_len; i++)
		cipher_text[*text_len + i] = padding_len;

	/* encrypting */
	int num = len / 8;
	for (i = 0; i < num; i++)
		encrypt((uint32_t *)(cipher_text + i * 8), (uint32_t *)key);

	*text_len = len;

	return cipher_text;
}

/*
 * @text_len: length of cipher_text or clear_text
 * @key: key, must be 16 bytes long
 * @return: clear_text and must be freed outside this function
 */
uint8_t *rtl_tea_decrypt(const uint8_t *cipher_text, int *text_len, const uint8_t *key)
{
	int i;

	/* the length must be multiple of 8 bytes */
	if (*text_len % 8 != 0)
		return NULL;

	uint8_t *clear_text = malloc(*text_len);
	if (!clear_text)
		return NULL;

	memcpy(clear_text, cipher_text, *text_len);

	/* decrypting */
	int num = *text_len / 8;
	for (i = 0; i < num; i++)
		decrypt((uint32_t *)(clear_text + i * 8), (uint32_t *)key);

	uint8_t padding_len = clear_text[*text_len - 1];

	if (padding_len >= 1 && padding_len <= 8) {
		*text_len -= padding_len;
	} else {
		free(clear_text);
		return NULL;
	}

	return clear_text;
}
