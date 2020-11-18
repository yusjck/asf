#ifndef _CRYPT_H
#define _CRYPT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void rc4(uint8_t *key, uint32_t keylen, uint8_t *buf, uint32_t buflen);
void rc5_key(uint8_t *pbKey, uint32_t dwLen);
void rc5_encrypt(void *dst, void *src, uint32_t len);
void rc5_decrypt(void *dst, void *src, uint32_t len);

#ifdef __cplusplus
}
#endif

#endif