#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define MAXBITS			15
#define MINOFFSET		0x01
#define MINMATCH		0x03
#define MAXMATCH		((1 << 24) + MINMATCH)
#define MAXWND			(1 << MAXBITS)
#define NIL				0xffff
#define M				3
#define MAX(a, b)		((a) > (b) ? (a) : (b))
#define MIN(a, b)		((a) < (b) ? (a) : (b))

typedef struct _LZ77_MATCHINFO
{
	uint32_t len;
	uint32_t off;
} LZ77_MATCHINFO;

typedef struct _LZ77_RUNSTATE
{
	uint32_t wsize;
	uint8_t *pwnd;
	uint32_t confine;
	uint16_t *head;
	uint16_t *prev;
	uint32_t nice;
} LZ77_RUNSTATE;

typedef struct _LZ77_IOSTATE
{
	uint8_t *pbuf;
	uint32_t bytenum;
	uint8_t bitnum;
	uint8_t codelen;
} LZ77_INPUTS, LZ77_OUTPUTS;

// ---------------------------------------------------------------------------
// 计算用二进制表示指定数值至少需要多少位
// ---------------------------------------------------------------------------
static uint8_t log2(uint32_t n)
{
	uint8_t c, i;

	if (n > 0xffff)
	{
		for (i = 16; n > ((uint32_t)-1 >> (sizeof(uint32_t) * 8 - i)); i++);
		return i;
	}

	if (n & 0xff00)
	{
		if (n & 0xf000)
		{
			if (n & 0xc000)
			{
				if (n & 0x8000)
				{
					c = 16;
				}
				else
				{
					c = 15;
				}
			}
			else
			{
				if (n & 0x2000)
				{
					c = 14;
				}
				else
				{
					c = 13;
				}
			}
		}
		else
		{
			if (n & 0x0c00)
			{
				if (n & 0x0800)
				{
					c = 12;
				}
				else
				{
					c = 11;
				}
			}
			else
			{
				if (n & 0x0200)
				{
					c = 10;
				}
				else
				{
					c = 9;
				}
			}
		}
	}
	else
	{
		if (n & 0x00f0)
		{
			if (n & 0x00c0)
			{
				if (n & 0x0080)
				{
					c = 8;
				}
				else
				{
					c = 7;
				}
			}
			else
			{
				if (n & 0x0020)
				{
					c = 6;
				}
				else
				{
					c = 5;
				}
			}
		}
		else
		{
			if (n & 0x000c)
			{
				if (n & 0x0008)
				{
					c = 4;
				}
				else
				{
					c = 3;
				}
			}
			else
			{
				if (n & 0x0002)
				{
					c = 2;
				}
				else
				{
					c = 1;
				}
			}
		}
	}

	return c;
}

// ---------------------------------------------------------------------------
// 输出指定长度的二进制位，最大长度为sizeof(uint32_t)
// ---------------------------------------------------------------------------
static void PutBits(LZ77_OUTPUTS *out, uint32_t v, int num)
{
	uint8_t *s = out->pbuf + out->bytenum;
	uint32_t i = 0;
	uint32_t temp = v & ~(-1 << num);

	do
	{
		s[i] &= ~(-1 << out->bitnum);
		s[i] |= (uint8_t)(temp << out->bitnum);
		if (8 - out->bitnum >= num)
			break;
		s[i + 1] = (uint8_t)(temp >> (8 - out->bitnum));
		temp >>= 8;
	} while ((++i << 3) < (uint32_t)num);

	out->bitnum += (uint8_t)num;
	out->bytenum += out->bitnum >> 3;
	out->bitnum &= 7;
}

// ---------------------------------------------------------------------------
// 获取指定长度的二进制位，最大长度为sizeof(uint32_t)
// ---------------------------------------------------------------------------
static uint32_t GetBits(LZ77_INPUTS *in, int num)
{
	uint8_t *s = in->pbuf + in->bytenum;
	uint32_t i = 0, v = 0;

	do
	{
		v |= (s[i] >> in->bitnum) << (i << 3);
		if (8 - in->bitnum >= num)
			break;
		v |= (s[i + 1] << (8 - in->bitnum)) << (i << 3);
	} while ((++i << 3) < (uint32_t)num);

	in->bitnum += (uint8_t)num;
	in->bytenum += in->bitnum >> 3;
	in->bitnum &= 7;

	return v & ~(-1 << num);
}

// ---------------------------------------------------------------------------
// 将指定位置开始的字节串添加到字典中
// ---------------------------------------------------------------------------
static void insert(LZ77_RUNSTATE *rs, uint32_t at, uint32_t len)
{
	uint32_t ins_h, ins_t;

	if (len == 1)
	{
		ins_h = *(uint16_t *)(rs->pwnd + at);
		rs->prev[at] = rs->head[ins_h];
		rs->head[ins_h] = (uint16_t)at;
		return;
	}

	if ((at + len) < MAXWND)
	{
		ins_t = -1;
		len += at--;

		while (++at != len)
		{
			ins_h = *(uint16_t *)(rs->pwnd + at);
			if ((ins_t - rs->head[ins_h]) <= 2)
				continue;
			ins_t = at;
			rs->prev[at] = rs->head[ins_h];
			rs->head[ins_h] = (uint16_t)at;
		}
	}
}

// ---------------------------------------------------------------------------
// 标志位定义：
// 长度：1，值：0，表示后面有一字节未压缩数据
// 长度：2，值：10，表示后面有一个匹配（变长偏移+变长长度）
// 长度：3，值：110，表示后面有一个匹配（7位偏移+1位长度，偏移为128时表示压缩流结束）
// 长度：3，值：111，表示后面有多个字节未压缩数据
// ---------------------------------------------------------------------------
#define CHARBITS1 4
#define CHARBITS2 7
#define CHARBITS3 16
#define CHARNUMS0 7
#define CHARNUMS1 ((1 << CHARBITS1) - 1 + (CHARNUMS0 + 1) - 2)
#define CHARNUMS2 ((1 << CHARBITS2) - 1 + (CHARNUMS1 + 1))
#define CHARNUMS3 ((1 << CHARBITS3) - 1 + (CHARNUMS2 + 1))

// ---------------------------------------------------------------------------
// 以压缩格式输出指定长度的字节串并针对格式长度进行优化
// ---------------------------------------------------------------------------
static void outcodec(LZ77_OUTPUTS *out, uint8_t *buffer, uint32_t length)
{
	uint32_t i, temp;

	if (length <= CHARNUMS0)
	{
		for (i = 0; i < length; i++)
		{
			// 逐字节输出，额外输出位(length)
			temp = 0x00 | (buffer[i] << 1);
			PutBits(out, temp, 1 + 8);			// 标志位(1)，数据位(8)
		}
	}
	else
	{
		if (length <= CHARNUMS1)
		{
			// 输出(0-13)表示有(0-13)+8个连续字节未压缩，额外输出位(7)
			temp = 0x07 | ((length - CHARNUMS0 - 1) << 3);
			PutBits(out, temp, 3 + CHARBITS1);	// 标志位(3)，数据位(4)
		}
		else if (length <= CHARNUMS1 * 2)
		{
			// 优化输出，最大额外输出位(14)
			outcodec(out, buffer, CHARNUMS1);
			outcodec(out, buffer + CHARNUMS1, length - CHARNUMS1);
			return;
		}
		else if (length <= CHARNUMS2)
		{
			// 输出(14)表示未压缩字节数由后面7位决定，额外输出位(14)
			temp = 0x07 | (14 << 3);
			PutBits(out, temp, 3 + CHARBITS1);	// 标志位(3)，数据位(4)

			temp = length - CHARNUMS1 - 1;
			PutBits(out, temp, CHARBITS2);		// 数据位(7)
		}
		else if (length <= CHARNUMS2 + CHARNUMS1)
		{
			// 优化输出，最大额外输出位(21)
			outcodec(out, buffer, CHARNUMS2);
			outcodec(out, buffer + CHARNUMS2, length - CHARNUMS2);
			return;
		}
		else
		{
			// 输出(15)表示未压缩字节数由后面两字节决定，额外输出位(23)
			temp = 0x07 | (15 << 3);
			PutBits(out, temp, 3 + CHARBITS1);	// 标志位(3)，数据位(4)

			// 输出(0-65535)+18个连续字节未压缩
			temp = length - CHARNUMS2 - 1;
			PutBits(out, temp, CHARBITS3);		// 数据位(16)
		}

		{
			uint8_t *s = out->pbuf + out->bytenum;
			uint8_t x = out->bitnum;

			temp = buffer[0];
			PutBits(out, temp, 8 - x);

			// 拷贝连续的未压缩字节
			for (i = 1; i < length; i++)
			{
				s[i] = buffer[i];
			}

			temp >>= 8 - x;
			out->bytenum += length - 1;
			PutBits(out, temp, x);
		}
	}
}

// ---------------------------------------------------------------------------
// 以压缩格式输出字节串匹配信息并针对信息长度进行优化
// ---------------------------------------------------------------------------
static void outcodex(LZ77_OUTPUTS *out, uint32_t offset, uint32_t length)
{
	uint8_t i = 0;
	uint32_t temp, m, n;

	switch (length)
	{
// 	case 1:
// 		temp = 0x03 | ((offset - MINOFFSET) << 3);
// 		PutBits(out, temp, 3 + 4);				// 标志位(3)，数据位(4)
// 		return;

	case 3:
		if (offset > 127)
			break;

	case 2:
		temp = 0x03 | ((offset - MINOFFSET) << 3);	// 短匹配优化
		temp |= (length - 2) << (3 + 7);
		PutBits(out, temp, 3 + 7 + 1);			// 标志位(3)，数据位(1+7)
		return;
	}

	// 写入变长匹配偏移
	temp = 0x01 | ((offset - MINOFFSET) << 2);
	PutBits(out, temp, 2 + out->codelen);		// 标志位(2)，数据位(log2(数据))

	length -= MINMATCH;
	m = 1 << (M - 1);

	// 计算匹配长度最少占用多少位
	do
	{
		n = ~(-1 << i++) << M;
		m <<= 1;
	} while ((m + n) <= length);

	// 写入匹配长度位数
	temp = ~(-1 << (i - 1));
	PutBits(out, temp, i);

	// 写入变长匹配长度
	temp = length - n;
	PutBits(out, temp, i + 3 - 1);
}

// ---------------------------------------------------------------------------
// 从当前字典中查找一个匹配字节串，成功返回1同时设置匹配字节串所在位置和长度
// ---------------------------------------------------------------------------
static int match(LZ77_RUNSTATE *rs, uint32_t strat, LZ77_MATCHINFO *mi)
{
	uint8_t *src, *s, *d, *c, *t;
	uint16_t index, *prev;
	uint32_t i, m = 0, n, nice, flag = 0;

	src = rs->pwnd;
	index = rs->head[*(uint16_t *)(src + strat)];		// 从字典中取出匹配信息

	if (NIL != index)
	{
		c = src + MIN(rs->confine, MAXMATCH);		// 限制最大匹配长度
		t = src + strat;
		m = MINMATCH - 1;
		prev = rs->prev;
		nice = rs->nice;

		do
		{
			// 开始寻找相同的字节串
			s = t;
			d = src + index;

			// 优化速度，一次循环比较8个字节
			while (s < (c - 8)
				&& *(uint16_t *)(s += 2) == *(uint16_t *)(d += 2)
				&& *(uint16_t *)(s += 2) == *(uint16_t *)(d += 2)
				&& *(uint16_t *)(s += 2) == *(uint16_t *)(d += 2)
				&& *(uint16_t *)(s += 2) == *(uint16_t *)(d += 2));

			while (s < c && *s == *d)
			{
				s++;
				d++;
			}

			if (s >= c)			// 达到限制长度了？
			{
				m = c - t;		// 如果是便是能找到的最好匹配了
				n = index;
				break;
			}

			i = s - t;

			if (m < i)			// 是否达到最小长度要求？
			{
				m = i;
				n = index;		// 记录下找到的信息
				if (m > nice)
					flag = 1;	// 如果达到预设的最优匹配长度则设置该标志，然后再次查找最优匹配
			}
			else if (flag)		// 如果已达到过预设的最优匹配长度则可以退出查找了
				break;
			index = prev[index];		// 取出字典中的下一次匹配记录
		} while (NIL != index);
	}

	if (MINMATCH <= m)			// 是否找到合适的匹配信息？
	{
		mi->len = m;
		mi->off = strat - n;
		return 1;
	}
	else
	{
		if (strat + 2 <= rs->confine)		// 检查所剩字节数是否足够2个字节
		{
			index = rs->head[*(uint16_t *)(src + strat)];		// 找不到合适的匹配时尝试查找2个字节的短匹配

			if (strat - index <= 127)		// 短匹配要求所在位置与当前位置之间不能超过127个字节
			{
				mi->len = 2;
				mi->off = strat - index;
				return 1;
			}
		}

		// 从前面16字节中查找1字节匹配
/*		for (i = 16; i > 0; i--)
		{
			if (*(src + strat - i) == *(src + strat))
			{
				mi->len = 1;
				mi->off = i;
				return 1;
			}
		}//*/
	}

	return 0;
}

// ---------------------------------------------------------------------------
// 压缩算法主循环，将输入数据压缩成一个独立的块
// ---------------------------------------------------------------------------
static uint32_t deflate(LZ77_RUNSTATE rs, uint8_t *dst, uint32_t *inbytes)
{
	LZ77_OUTPUTS out, prev_out;
	uint32_t strstart = 0, prev_start = 0, count = 0, prev_count = 0;
	LZ77_MATCHINFO mi;

	out.pbuf = dst;
	out.bytenum = 0;
	out.bitnum = 0;
	out.codelen = 1;
	prev_out = out;

	memset(rs.head, NIL, sizeof(uint16_t) * 65536);

	do
	{
		if (match(&rs, strstart, &mi))
		{
			if (count > 0)
			{
				outcodec(&out, rs.pwnd + strstart - count, count);		// 输出无匹配字节
				count = 0;
			}

			if ((uint32_t)(1 << out.codelen) <= (uint32_t)(strstart - MINOFFSET))
				out.codelen = log2(strstart - MINOFFSET);

			insert(&rs, strstart, mi.len);						// 更新字典记录
			outcodex(&out, mi.off, mi.len);						// 输出压缩代码
			strstart += mi.len;
		}
		else
		{
			insert(&rs, strstart, 1);							// 更新字典记录
			count++;											// 增加无匹配字节数量
			strstart += 1;
		}

		if (strstart - prev_start >= 0x1000)					// 跟踪压缩率变化
		{
			// 压缩后的数据是否比原始数据大？
			if (strstart - prev_start + 4 < out.bytenum - prev_out.bytenum)
			{
				// 不压缩直接输出原始数据
				out = prev_out;
				outcodec(&out, rs.pwnd + prev_start - prev_count, strstart - prev_start + prev_count);
				count = 0;
			}

			prev_out = out;
			prev_start = strstart;
			prev_count = count;
		}
	} while (strstart < rs.wsize);

	if (count > 0)
	{
		outcodec(&out, rs.pwnd + strstart - count, count);		// 输出无匹配字节
	}

	// 压缩后的数据是否比原始数据大？
	if (strstart - prev_start + 4 < out.bytenum - prev_out.bytenum)
	{
		// 不压缩直接输出原始数据
		out = prev_out;
		outcodec(&out, rs.pwnd + prev_start - prev_count, strstart - prev_start + prev_count);
	}

	// 压缩后的全部数据是否比原始数据大？
	if (strstart + 4 < out.bytenum)
	{
		// 直接输出全部数据
		out.pbuf = dst;
		out.bytenum = 0;
		out.bitnum = 0;
		out.codelen = 1;
		outcodec(&out, rs.pwnd, strstart);
	}

	outcodex(&out, 128, 2);		// 输出压缩流结束标记

	if (out.bitnum)
		out.bytenum++;

	*inbytes = strstart;

	return out.bytenum;
}

// ---------------------------------------------------------------------------
// 解压算法，每次处理一个压缩块
// ---------------------------------------------------------------------------
static uint32_t inflate(uint8_t *src, uint8_t *dst, uint32_t len, uint32_t *inbytes)
{
	uint32_t offset, length;
	uint8_t i, t;
	uint8_t *out, *s;
	LZ77_INPUTS in;

	in.pbuf = src;
	in.bytenum = 0;
	in.bitnum = 0;
	in.codelen = 1;
	out = dst;

	while (in.bytenum < len)
	{
		if (!GetBits(&in, 1))			// 0表示有一个未压缩的字节
		{
			*out++ = (uint8_t)GetBits(&in, 8);	// 输出一个未压缩字节
		}
		else
		{
			if (!GetBits(&in, 1))		// 10表示有一个长匹配
			{
				if ((uint32_t)(1 << in.codelen) <= (uint32_t)(out - dst - MINOFFSET))
					in.codelen = log2(out - dst - MINOFFSET);
				offset = GetBits(&in, in.codelen) + MINOFFSET;

				for (i = 0; GetBits(&in, 1); i++);	// 计算匹配长度位数
				length = GetBits(&in, i + M);
				length += (~(-1 << i) << M) + MINMATCH;		// 计算匹配长度

				do
				{
					*out = *(out - offset);
					out++;
				} while (--length);
			}
			else
			{
				if (!GetBits(&in, 1))	// 110表示有一个短匹配
				{
					offset = GetBits(&in, 7) + MINOFFSET;
					length = GetBits(&in, 1) + 2;

					if (offset == 128)	// offset值128为压缩流结束标志
						break;

					do
					{
						*out = *(out - offset);	// 解压短匹配
						out++;
					} while (--length);
				}
				else					// 111表示有一个未压缩的字节串
				{
					length = GetBits(&in, CHARBITS1);

					switch (length)
					{
					case 14:
						length = GetBits(&in, CHARBITS2);	// 长度为14时表示实际长度用后面的7位记录
						length += CHARNUMS1 + 1;
						break;
					case 15:
						length = GetBits(&in, CHARBITS3);	// 长度为15时表示实际长度用后面的16位记录
						length += CHARNUMS2 + 1;
						break;
					default:
						length += CHARNUMS0 + 1;
						break;
					}

					s = in.pbuf + in.bytenum;
					offset = 1;
					i = in.bitnum;

					do
					{
						out[offset] = s[offset];	// 还原字节串
					} while (++offset < length);

					t = (uint8_t)GetBits(&in, 8 - i);
					in.bytenum += length - 1;
					t |= (uint8_t)(GetBits(&in, i) << (8 - i));
					*out = t;
					out += length;
				}
			}
		}
	}

	*inbytes = in.bytenum;
	*inbytes += in.bitnum == 0 ? 0 : 1;

	return out - dst;
}

// ---------------------------------------------------------------------------
// LZ77压缩算法
// dst:压缩输出缓冲区
// src:原始数据
// len:原始数据长度
// level:压缩等级(0 - 4)
// return 压缩后数据大小
// ---------------------------------------------------------------------------
int Lz77Compress(void *dst, const void *src, uint32_t len, int level)
{
	LZ77_RUNSTATE rs;
	uint32_t m, n, count = 0;

	if (len <= 0)
		return 0;

	if (!src || !dst)
		return -1;

	// 设置压缩等级，等级越高引擎会花越多时间去查找一个更长的匹配
	switch (level)
	{
	case 0:
		rs.nice = 3;
		break;
	case 1:
		rs.nice = 30;
		break;
	case 2:
		rs.nice = 70;
		break;
	case 3:
		rs.nice = 150;
		break;
	case 4:
		rs.nice = -1;
		break;
	}

	// 为压缩字典分配内存空间
	rs.prev = (uint16_t *)malloc(sizeof(uint16_t) * 65536);
	rs.head = (uint16_t *)malloc(sizeof(uint16_t) * 65536);

	if (!rs.prev || !rs.head)
	{
		free(rs.head);
		free(rs.prev);
		return -1;
	}

	do
	{
		rs.wsize = MIN(len, MAXWND);
		rs.pwnd = (uint8_t *)src;
		// rs.confine = len;
		rs.confine = MIN(len, MAXWND);
		n = deflate(rs, dst, &m);
		len -= m;
		src = (uint8_t *)src + m;
		dst = (uint8_t *)dst + n;
		count += n;
	} while (len > 0);

	free(rs.head);
	free(rs.prev);

	return count;
}

// ---------------------------------------------------------------------------
// LZ77解压算法
// dst:解压输出缓冲区
// src:压缩数据
// len:压缩数据长度
// return 解压后数据大小
// ---------------------------------------------------------------------------
int Lz77Decompress(void *dst, const void *src, uint32_t len)
{
	uint32_t c = 0, i, o, a = 0;

	if (len == 0)
		return 0;

	if (!src || !dst)
		return -1;

	do
	{
		o = inflate((uint8_t *)src, dst, len, &i);
		src = (uint8_t *)src + i;
		dst = (uint8_t *)dst + o;
		len -= i;
		c += o;
	} while (len > 0);

	return c;
}
