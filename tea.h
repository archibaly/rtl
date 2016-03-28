#ifndef _TEA_H_
#define _TEA_H_

#include <stdint.h>

int tea_encrypt(uint8_t *src, int size, const uint8_t *key);
int tea_decrypt(uint8_t *src, int size, const uint8_t *key);

#endif /* _TEA_H_ */
