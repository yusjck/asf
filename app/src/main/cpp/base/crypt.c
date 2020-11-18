#include <string.h>
#include "crypt.h"

#define RC5_R 12
#define RC5_P 0xb7e15163
#define RC5_Q 0x9e3779b9
#define ROTL(x, c) (((x) << ((c) & 31)) | ((x) >> (32 - ((c) & 31))))
#define ROTR(x, c) (((x) >> ((c) & 31)) | ((x) << (32 - ((c) & 31))))

static uint32_t rc5_sbox[RC5_R * 2 + 2];

#define rc4_swap(a, b) {uint8_t t; t = *a; *a = *b; *b = t;}
// 使用RC4算法对数据进行加解密变换
void rc4(uint8_t *key, uint32_t keylen, uint8_t *buf, uint32_t buflen)
{
	uint32_t i, j, k;
	uint8_t x, y;
	uint8_t rc4_sbox[256];

	for (i = 0; i < 256; i++)
		rc4_sbox[i] = (uint8_t)i;

	j = k = 0;

	for (i = 0; i < 256; i++)
	{
		j += key[k] + rc4_sbox[i];

		if (++k >= keylen)
			k = 0;

		rc4_swap(&rc4_sbox[i], &rc4_sbox[j & 255]);
	}

	x = y = 0;

	for (i = 0; i < buflen; i++)
	{
		y += rc4_sbox[++x];
		rc4_swap(&rc4_sbox[x], &rc4_sbox[y]);
		j = rc4_sbox[x] + rc4_sbox[y];
		buf[i] ^= rc4_sbox[(uint8_t)j];
	}
}
#undef rc4_swap

void rc5_key(uint8_t *pbKey, uint32_t dwLen)
{
	uint32_t i, j, k, A, B;
	uint32_t *S;
	uint32_t L[16];
	uint32_t SL, LL;

	SL = (RC5_R + 1) * 2;
	LL = (dwLen + 3) >> 2;
	S = rc5_sbox;
	L[dwLen >> 2] = 0;
	memcpy((uint8_t *)L, pbKey, dwLen);

	S[0] = RC5_P;

	for (i = 1; i < SL; i++)
	{
		S[i] = S[i - 1] + RC5_Q;
	}

	i = (SL > LL ? SL : LL) * 3;
	A = B = j = k = 0;

	for (; i > 0; i--)
	{
		A = S[j] = ROTL(S[j] + (A + B), 3);
		B = L[k] = ROTL(L[k] + (A + B), (A + B));
		if (++j >= SL) j = 0;
		if (++k >= LL) k = 0;
	}
}

static void rc5_encrypt1(uint8_t *pOut, uint8_t *pIn)
{
	uint32_t i, A, B;
	uint32_t *S;

	S = rc5_sbox;
	A = ((uint32_t *)pIn)[0] + S[0];
	B = ((uint32_t *)pIn)[1] + S[1];

	for (i = 1; i <= RC5_R; i++)
	{
		A = ROTL(A ^ B, B) + S[2 * i];
		B = ROTL(B ^ A, A) + S[2 * i + 1];
	}

	((uint32_t *)pOut)[0] = A;
	((uint32_t *)pOut)[1] = B;
}

static void rc5_decrypt1(uint8_t *pOut, uint8_t *pIn)
{
	uint32_t i, A, B;
	uint32_t *S;

	S = rc5_sbox;
	B = ((uint32_t *)pIn)[1];
	A = ((uint32_t *)pIn)[0];

	for (i = RC5_R; i > 0; i--)
	{
		B = ROTR(B - S[2 * i + 1], A) ^ A;
		A = ROTR(A - S[2 * i], B) ^ B;
	}

	((uint32_t *)pOut)[1] = B - S[1];
	((uint32_t *)pOut)[0] = A - S[0];
}

void rc5_encrypt(void *dst, void *src, uint32_t len)
{
	uint32_t i, m, n;

	if (len < 8)
	{
		for (i = 0; i < len; i++)
			((uint8_t *)dst)[i] = ((uint8_t *)src)[i] ^ (uint8_t)rc5_sbox[i];
		return;
	}

	if (len & 7)
	{
		n = (len & 7) + 8;
		m = len - n;
	}
	else
	{
		n = 0;
		m = len;
	}

	for (i = 0; i < m; i += 8)
	{
		rc5_encrypt1((uint8_t *)dst + i, (uint8_t *)src + i);
	}

	if (n)
	{
		memcpy((uint8_t *)dst + i, (uint8_t *)src + i, n);
		rc5_encrypt1((uint8_t *)dst + i, (uint8_t *)dst + i);
		rc5_encrypt1((uint8_t *)dst + i + n - 8, (uint8_t *)dst + i + n - 8);
	}
}

void rc5_decrypt(void *dst, void *src, uint32_t len)
{
	uint32_t i, m, n;

	if (len < 8)
	{
		for (i = 0; i < len; i++)
			((uint8_t *)dst)[i] = ((uint8_t *)src)[i] ^ (uint8_t)rc5_sbox[i];
		return;
	}

	if (len & 7)
	{
		n = (len & 7) + 8;
		m = len - n;
	}
	else
	{
		n = 0;
		m = len;
	}

	for (i = 0; i < m; i += 8)
	{
		rc5_decrypt1((uint8_t *)dst + i, (uint8_t *)src + i);
	}

	if (n)
	{
		memcpy((uint8_t *)dst + i, (uint8_t *)src + i, n);
		rc5_decrypt1((uint8_t *)dst + i + n - 8, (uint8_t *)dst + i + n - 8);
		rc5_decrypt1((uint8_t *)dst + i, (uint8_t *)dst + i);
	}
}
