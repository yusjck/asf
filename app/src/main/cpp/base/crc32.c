#include "crc32.h"

static int crc_table_empty = 1;
static uint32_t crc_table[256];

static void make_crc_table()
{
	uint32_t c, poly;
	int n, k;
	static const uint8_t p[] = {0,1,2,4,5,7,8,10,11,12,16,22,23,26};

	/* make exclusive-or pattern from polynomial (0xedb88320L) */
	poly = 0L;

	for (n = 0; n < sizeof(p) / sizeof(uint8_t); n++)
		poly |= 1L << (31 - p[n]);

	for (n = 0; n < 256; n++)
	{
		c = (uint32_t)n;

		for (k = 0; k < 8; k++)
			c = c & 1 ? poly ^ (c >> 1) : c >> 1;
		crc_table[n] = c;
	}
}

/* ========================================================================= */
#define DO1(buf) crc = crc_table[((int)crc ^ (*buf++)) & 0xff] ^ (crc >> 8);
#define DO2(buf)  DO1(buf); DO1(buf);
#define DO4(buf)  DO2(buf); DO2(buf);
#define DO8(buf)  DO4(buf); DO4(buf);
/* ========================================================================= */

uint32_t do_crc32(const void *buf, size_t len)
{
	uint32_t crc = 0xffffffffL;
	uint8_t *buf1 = (uint8_t *)buf;

	if (crc_table_empty)
	{
		make_crc_table();
		crc_table_empty = 0;
	}

	while (len >= 8)
	{
		DO8(buf1);
		len -= 8;
	}

	while (len--)
		DO1(buf1);

	return crc ^ 0xffffffffL;
}
