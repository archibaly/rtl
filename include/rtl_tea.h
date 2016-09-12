#ifndef _RTL_TEA_H_
#define _RTL_TEA_H_

#include <stdint.h>

#define RTL_TEA_KEY	"l(91#%2*!<6&@89^"

int rtl_tea_encrypt(uint8_t *src, int size, const uint8_t *key);
int rtl_tea_decrypt(uint8_t *src, int size, const uint8_t *key);

#endif /* _RTL_TEA_H_ */
