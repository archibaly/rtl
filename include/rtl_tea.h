#ifndef _RTL_TEA_H_
#define _RTL_TEA_H_

#include <stdint.h>

#define RTL_TEA_KEY	"l(9A#%2*!<6&@!9^"

uint8_t *rtl_tea_encrypt(const uint8_t *clear_text, int *text_len, const uint8_t *key);
uint8_t *rtl_tea_decrypt(const uint8_t *cipher_text, int *text_len, const uint8_t *key);

#endif /* _RTL_TEA_H_ */
