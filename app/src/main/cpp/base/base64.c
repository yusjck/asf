#include <stdint.h>

static char base64_alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";

uint32_t Base64Encode(const void *src, uint32_t srclen, char *dst, uint32_t dstlen)
{
	uint8_t *p1 = (uint8_t *)src;
	char *p2 = dst;
	uint32_t i, v;

	for (i = 0; i < srclen; i += 3)
	{
		if ((uint32_t)(p2 - dst + 4) >= dstlen)
			break;

		switch (srclen - i)
		{
		case 1:
			v = (p1[i] << 16);
			*p2++ = base64_alphabet[v >> 18];
			*p2++ = base64_alphabet[(v >> 12) & 63];
			*p2++ = base64_alphabet[64];
			*p2++ = base64_alphabet[64];
			break;

		case 2:
			v = (p1[i] << 16) | (p1[i + 1] << 8);
			*p2++ = base64_alphabet[v >> 18];
			*p2++ = base64_alphabet[(v >> 12) & 63];
			*p2++ = base64_alphabet[(v >> 6) & 63];
			*p2++ = base64_alphabet[64];
			break;

		default:
			v = (p1[i] << 16) | (p1[i + 1] << 8) | p1[i + 2];
			*p2++ = base64_alphabet[v >> 18];
			*p2++ = base64_alphabet[(v >> 12) & 63];
			*p2++ = base64_alphabet[(v >> 6) & 63];
			*p2++ = base64_alphabet[v & 63];
			break;
		}
	}

	*p2++ = '\0';

	return (uint32_t)(p2 - dst - 1);
}

uint32_t Base64Decode(const char *src, uint32_t srclen, void *dst, uint32_t dstlen)
{
	const char *p1 = src;
	uint8_t *p2 = (uint8_t *)dst;
	uint32_t i, n, v = 0;
	uint8_t base64_table[256];

	for (i = 0; i < sizeof(base64_table); i++)
		base64_table[i] = 255;

	for (i = 0; i < 64; i++)
		base64_table[base64_alphabet[i]] = (char)i;

	for (i = 0, n = 0; i < srclen; i++)
	{
		if (base64_table[p1[i]] == 255)
			break;

		if ((uint32_t)(p2 - (uint8_t *)dst) >= dstlen)
			break;

		v = base64_table[p1[i]] | (v << 6);
		n += 6;

		if (n >= 8)
		{
			n -= 8;
			*p2++ = (uint8_t)(v >> n);
		}
	}

	return (uint32_t)(p2 - (uint8_t *)dst);
}
