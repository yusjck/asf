#ifndef _FILEUTIL_H
#define _FILEUTIL_H

#include <stdint.h>
#include "buffer.h"

#define MAX_PATH 260

bool WriteDataToFile(const char *fileName, const void *data, size_t dataSize, bool append = false);
bool CalculateFileCrc32(const char *fileName, uint32_t *fileCrcValue);

class FileBuffer : public CBuffer
{
public:
	bool LoadFromFile(const char *fileName);
	bool SaveToFile(const char *fileName);
	bool LoadFromPrivateFile(const char *fileName);
	bool SaveToPrivateFile(const char *fileName, bool compress = true);
	bool ConvertToUtf8();
	uint32_t CalculateCrc32();
	FileBuffer();
	virtual ~FileBuffer();

private:
	bool DecodePrivateFile(FILE *srcFile, size_t fileSize);
};

#endif
