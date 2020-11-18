#ifndef _LZ77_H
#define _LZ77_H

#ifdef __cplusplus
extern "C" {
#endif

	/*
	 * 注意：函数接口与类接口不能混用，因为类接口在压缩时使用了额外的4字节保存原始数据长度
	 */

// ---------------------------------------------------------------------------
// LZ77压缩算法
// dst:压缩输出缓冲区
// src:原始数据
// len:原始数据长度
// level:压缩等级(0 - 4)
// return 压缩后数据大小
// ---------------------------------------------------------------------------
int Lz77Compress(void *dst, const void *src, uint32_t len, int level);

// ---------------------------------------------------------------------------
// LZ77解压算法
// dst:解压输出缓冲区
// src:压缩数据
// len:压缩数据长度
// return 解压后数据大小
// ---------------------------------------------------------------------------
int Lz77Decompress(void *dst, const void *src, uint32_t len);

#ifdef __cplusplus
}

#include "buffer.h"

class CLz77 : public CBuffer
{
private:
	CLz77()
	{
	}

public:
	~CLz77()
	{
	}

	static CLz77 Compress(const void *src, int len, int level)
	{
		/*
		 * 分配缓冲区用于输出压缩数据
		 * Lz77引擎将数据分成多个64K字节的块，每个块压缩后最大为64K+4字节
		 * 为防止数据冗余过低压缩后体积大于未压缩前的数据，
		 * 分配压缩缓冲区时为每个块多分配4字节并为全部数据多分配64K空间
		 */
		size_t compressBufferSize = len + len / 0x10000 * 4 + 0x10000;
		CLz77 lz77;
		void *compressBuffer = lz77.GetBuffer(compressBufferSize);
		if (compressBuffer)
		{
			// 压缩后输出数据的前4个字节为原始数据长度，解压时可以用于预分配缓冲区
			*(uint32_t *)compressBuffer = len;
			uint32_t compressedDataSize = ::Lz77Compress((uint8_t *)compressBuffer + sizeof(uint32_t), src, len, level);
			compressedDataSize += sizeof(uint32_t);

			// 调整缓冲区大小为实际输出的数据长度
			lz77.ReleaseBuffer(compressedDataSize);
		}
		return lz77;
	}

	static CLz77 Decompress(const void *src, int len)
	{
		// 根据原始数据长度来分配缓冲区，这里需要额外保留4字节以防止溢出
		uint32_t decompressBufferSize = *(uint32_t *)src + sizeof(uint32_t);
		CLz77 lz77;
		void *decompressBuffer = lz77.GetBuffer(decompressBufferSize);
		if (decompressBuffer)
		{
			uint32_t decompressedDataSize = ::Lz77Decompress(decompressBuffer, (uint8_t *)src + sizeof(uint32_t), len - sizeof(uint32_t));
			lz77.ReleaseBuffer(decompressedDataSize);	// 调整为实际输出数据长度
		}
		return lz77;
	}

	operator const void *() {return GetBuffer();}
};
#endif

#endif
