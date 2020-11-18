#ifndef _BASE64_H
#define _BASE64_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint32_t Base64Encode(const void *src, uint32_t srclen, char *dst, uint32_t dstlen);
uint32_t Base64Decode(const char *src, uint32_t srclen, void *dst, uint32_t dstlen);

#ifdef __cplusplus
}

#include "buffer.h"

class CBase64 : public CBuffer
{
private:
	CBase64()
	{
	}

public:
	~CBase64()
	{
	}

	static CBase64 Encode(const void *src, uint32_t srclen)
	{
		uint32_t bufferSize = srclen * 4 / 3 + 4;		// base64编码后数据长度大约为原始长度的4/3倍
		CBase64 base64;
		void *buffer = base64.GetBuffer(bufferSize);
		if (buffer)
		{
			memset(buffer, 0, bufferSize);
			uint32_t dataLength = Base64Encode(src, srclen, (char *)buffer, bufferSize);
			base64.ReleaseBuffer(dataLength);		// 调整为实际输出数据长度
		}
		return base64;
	}

	static CBase64 Decode(const char *src, uint32_t srclen)
	{
		uint32_t bufferSize = srclen * 3 / 4 + 2;
		CBase64 base64;
		void *buffer = base64.GetBuffer(bufferSize);
		if (buffer)
		{
			memset(buffer, 0, bufferSize);
			uint32_t dataLength = Base64Decode(src, srclen, buffer, bufferSize);
			base64.ReleaseBuffer(dataLength);		// 调整为实际输出数据长度
		}
		return base64;
	}

	static CBase64 Encode(const char *src)
	{
		return Encode(src, (uint32_t)strlen(src));
	}

	static CBase64 Decode(const char *src)
	{
		return Decode(src, (uint32_t)strlen(src));
	}

	operator const void *() {return GetBuffer();}
	operator const char *() {return (const char *)GetBuffer();}
};
#endif

#endif