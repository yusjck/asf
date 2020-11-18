#ifndef STRUTIL_H
#define STRUTIL_H

#include <string>
#include <vector>

#ifndef _countof
#define _countof(array) (sizeof(array)/sizeof(array[0]))
#endif

std::string format(const char *fmt, ...);
std::vector<std::string> split(const std::string &src, std::string delimit, std::string null_subst = "");
std::string &trim(std::string &s);
void string_replace(std::string &s1, const std::string &s2, const std::string &s3);
int HexStringToBinary(uint8_t *pBuffer, uint32_t nBufferSize, const char *pHexString);
int BinaryToHexString(char *pHexString, uint32_t nMaxSize, const uint8_t *pBinary, uint32_t nBinarySize);

bool IsUtf8Text(const char *str, size_t len);
int Ucs2ToUtf8(const uint16_t *ucs2String, int cchUcs2Char, char *utf8String, int cchUtf8Char);
int Utf8ToUcs2(const char *utf8String, int cchUtf8Char, uint16_t *ucs2String, int cchUcs2Char);

#endif
