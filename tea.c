#include "tea.h"

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
 * @src: must be multiple of 8 bytes
 * @size: size of src
 * @key: key, must be 16 bytes long
 * @return: the bytes of encrypted data
 */
int tea_encrypt(uint8_t *src, int size, const uint8_t *key)
{
	int a = 0;
	int i = 0;
	int num = 0;
	
	/* if the size of data is not multiple of 8 bytes, just add zeros */
	a = size % 8;
	if (a != 0)
		for (i = 0; i < 8 - a; i++)
			src[size++] = 0;

	/* encrypting */
	num = size / 8;
	for (i = 0; i < num; i++)
		encrypt((uint32_t *)(src + i * 8), (uint32_t *)key);
	
	return size;
}

/*
 * @src: must be multiple of 8 bytes
 * @size: size of src
 * @key: key, must be 16 bytes long
 * @return: the bytes of data
 */
int tea_decrypt(uint8_t *src, int size, const uint8_t *key)
{
	int i = 0;
	int num = 0;
	
	/* the size is multiple of 8 bytes or not */
	if (size % 8 != 0)
		return 0;
	
	/* decrypting */
	num = size / 8;
	for (i = 0; i < num; i++)
		decrypt((uint32_t *)(src + i * 8), (uint32_t *)key);
	
	return size;
}
