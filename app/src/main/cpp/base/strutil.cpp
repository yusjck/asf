#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <vector>
#include <string>
#include <algorithm>
#include "strutil.h"

std::string format(const char *fmt, ...)
{
	char buf[1024];
	char *pbuf = buf;
	int pbuf_size = sizeof(buf);
	int len = 0;
	int again = 0;
	va_list ap;
	va_start(ap, fmt);
	do
	{
		if (again)
		{
#ifdef _MSC_VER
			pbuf_size += sizeof(buf);
#else
			pbuf_size = len + 1;
#endif
			pbuf = (char *)((pbuf == buf) ? (char *)malloc(pbuf_size)
				: (char *)realloc(pbuf, pbuf_size));
		}
		len = vsnprintf(pbuf, pbuf_size, fmt, ap);
	} while ((again = ((len < 0) || (pbuf_size <= len))) != 0);
#ifdef _MSC_VER
	pbuf[len] = '\0';
#endif
	std::string str(pbuf);
	if (pbuf != buf)
	{
		free(pbuf);
	}
	va_end(ap);
	return str;
}

std::vector<std::string> split(const std::string &src, std::string delimit, std::string null_subst)
{
	if (delimit.empty()) throw "split: empty string\0";
	std::vector<std::string> v;
	const std::string::size_type npos = -1;
	std::string::size_type deli_len = delimit.size();
	int index = 0, last_search_position = 0;
	while ((index = src.find(delimit, last_search_position)) != npos)
	{
		if (index == last_search_position)
			v.push_back(null_subst);
		else
			v.push_back(src.substr(last_search_position, index - last_search_position));
		last_search_position = index + deli_len;
	}
	std::string last_one = src.substr(last_search_position);
	v.push_back(last_one.empty() ? null_subst : last_one);
	return v;
}

std::string &trim(std::string &s)
{
	if (s.empty())
		return s;

	s.erase(0, s.find_first_not_of(" "));
	s.erase(s.find_last_not_of(" ") + 1);
	return s;
}

void string_replace(std::string &s1, const std::string &s2, const std::string &s3)
{
	std::string::size_type pos = 0;
	std::string::size_type a = s2.size();
	std::string::size_type b = s3.size();
	while ((pos = s1.find(s2, pos)) != std::string::npos)
	{
		s1.replace(pos, a, s3);
		pos += b;
	}
}

int HexStringToBinary(uint8_t *pBuffer, uint32_t nBufferSize, const char *pHexString)
{
	uint32_t nStringLength = (uint32_t)strlen(pHexString);
	uint32_t i;
	for (i = 0; i < nStringLength / 2; i++)
	{
		if (i >= nBufferSize)
			break;
		if (pHexString[i * 2] == 0 || pHexString[i * 2 + 1] == 0)
			break;
		uint8_t byte = 0;
		if ((uint8_t)(pHexString[i * 2] - '0') < 10)
			byte = pHexString[i * 2] - '0';
		else if ((uint8_t)(pHexString[i * 2] - 'A') < 26)
			byte = pHexString[i * 2] - 'A' + 10;
		else if ((uint8_t)(pHexString[i * 2] - 'a') < 26)
			byte = pHexString[i * 2] - 'a' + 10;
		byte <<= 4;
		if ((uint8_t)(pHexString[i * 2 + 1] - '0') < 10)
			byte |= pHexString[i * 2 + 1] - '0';
		else if ((uint8_t)(pHexString[i * 2 + 1] - 'A') < 26)
			byte |= pHexString[i * 2 + 1] - 'A' + 10;
		else if ((uint8_t)(pHexString[i * 2 + 1] - 'a') < 26)
			byte |= pHexString[i * 2 + 1] - 'a' + 10;
		pBuffer[i] = byte;
	}
	return i;
}

int BinaryToHexString(char *pHexString, uint32_t nMaxSize, const uint8_t *pBinary, uint32_t nBinarySize)
{
	uint32_t i;
	for (i = 0; i < nBinarySize; i++)
	{
		if (i * 2 + 1 >= nMaxSize)
			break;
		pHexString[i * 2] = "0123456789ABCDEF"[pBinary[i] >> 4];
		pHexString[i * 2 + 1] = "0123456789ABCDEF"[pBinary[i] & 15];
	}
	pHexString[i * 2] = '\0';
	return i * 2;
}

bool IsUtf8Text(const char *str, size_t len)
{
	bool bAllAscii = true;

	while (len)
	{
		uint8_t ch = *str++;
		len--;
		if (ch & 0x80)
		{
			size_t count = 0;
			do
			{
				ch <<= 1;
				count++;
			} while (ch & 0x80);

			if (count < 2 || count > 6)
				return false;

			if (len < count - 1)
				return false;

			for (size_t i = 0; i < count - 1; i++)
			{
				if ((*str & 0xc0) != 0x80)
					return false;
				str++;
				len--;
			}

			bAllAscii = false;
		}
	}

	return !bAllAscii;
}

static size_t ucs2len(const uint16_t *str)
{
	size_t len = 0;
	while (str[len]) len++;
	return len;
}

#define UTF8_ONE_START      (0xOOO1)
#define UTF8_ONE_END        (0x007F)
#define UTF8_TWO_START      (0x0080)
#define UTF8_TWO_END        (0x07FF)
#define UTF8_THREE_START    (0x0800)
#define UTF8_THREE_END      (0xFFFF)

int Ucs2ToUtf8(const uint16_t *ucs2String, int cchUcs2Char, char *utf8String, int cchUtf8Char)
{
	const uint16_t *ucs2StringPtr = ucs2String;
	const uint16_t *ucs2StringEnd = ucs2String + (cchUcs2Char == -1 ? ucs2len(ucs2String) : cchUcs2Char);
	uint8_t *utf8StringPtr = (uint8_t *)utf8String;
	uint8_t *utf8StringEnd = (uint8_t *)utf8String + cchUtf8Char;

	while (ucs2StringPtr < ucs2StringEnd)
	{
		if (*ucs2StringPtr <= UTF8_ONE_END && utf8StringPtr < utf8StringEnd)
		{
			// 0000 - 007F  0xxxxxxx
			*utf8StringPtr++ = (char)*ucs2StringPtr;
		}
		else if (*ucs2StringPtr >= UTF8_TWO_START && *ucs2StringPtr <= UTF8_TWO_END && utf8StringPtr + 1 < utf8StringEnd)
		{
			// 0080 - 07FF 110xxxxx 10xxxxxx
			*utf8StringPtr++ = (*ucs2StringPtr >> 6) | 0xC0;
			*utf8StringPtr++ = (*ucs2StringPtr & 0x3F) | 0x80;
		}
		else if (*ucs2StringPtr >= UTF8_THREE_START && *ucs2StringPtr <= UTF8_THREE_END && utf8StringPtr + 2 < utf8StringEnd)
		{
			// 0800 - FFFF 1110xxxx 10xxxxxx 10xxxxxx
			*utf8StringPtr++ = (*ucs2StringPtr >> 12) | 0xE0;
			*utf8StringPtr++ = ((*ucs2StringPtr >> 6) & 0x3F) | 0x80;
			*utf8StringPtr++ = (*ucs2StringPtr & 0x3F) | 0x80;
		}
		else
		{
			break;
		}
		ucs2StringPtr++;
	}
	if (utf8StringPtr < utf8StringEnd)
		*utf8StringPtr = 0;

	return (int)(utf8StringPtr - (uint8_t *)utf8String);
}

int Utf8ToUcs2(const char *utf8String, int cchUtf8Char, uint16_t *ucs2String, int cchUcs2Char)
{
	uint8_t *utf8StringPtr = (uint8_t *)utf8String;
	uint8_t *utf8StringEnd = (uint8_t *)utf8String + (cchUtf8Char == -1 ? strlen(utf8String) : cchUtf8Char);
	uint16_t *ucs2StringPtr = ucs2String;
	uint16_t *ucs2StringEnd = ucs2String + cchUcs2Char;

	while (utf8StringPtr < utf8StringEnd && ucs2StringPtr < ucs2StringEnd)
	{
		if (*utf8StringPtr >= 0xE0 && *utf8StringPtr <= 0xEF)
		{
			// 0800 - FFFF 1110xxxx 10xxxxxx 10xxxxxx
			*ucs2StringPtr = ((*utf8StringPtr++ & 0xEF) << 12);
			*ucs2StringPtr |= ((*utf8StringPtr++ & 0x3F) << 6);
			*ucs2StringPtr |= (*utf8StringPtr++ & 0x3F);
		}
		else if (*utf8StringPtr >= 0xC0 && *utf8StringPtr <= 0xDF)
		{
			// 0080 - 07FF 110xxxxx 10xxxxxx
			*ucs2StringPtr = ((*utf8StringPtr++ & 0x1F) << 6);
			*ucs2StringPtr |= (*utf8StringPtr++ & 0x3F);
		}
		else if (*utf8StringPtr >= 0 && *utf8StringPtr <= 0x7F)
		{
			// 0000 - 007F  0xxxxxxx
			*ucs2StringPtr = *utf8StringPtr++;
		}
		else
		{
			break;
		}
		ucs2StringPtr++;
	}
	if (ucs2StringPtr < ucs2StringEnd)
		*ucs2StringPtr = 0;

	return (int)(ucs2StringPtr - ucs2String);
}
