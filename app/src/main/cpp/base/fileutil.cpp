#include "crc32.h"
#include "lz77.h"
#include "fileutil.h"
#include "strutil.h"

#define FILE_SIGN 0x837076A1

typedef struct _FILE_HEADER {
	uint32_t FileSign;
	uint32_t DataSize;
	uint32_t DataCrc;
	uint32_t Compressed : 1;
	uint32_t EncryptType : 7;
	uint32_t EncryptKey : 24;
	uint32_t OriginalDataSize;
	uint8_t DataStream[];
} FILE_HEADER, *PFILE_HEADER;

FileBuffer::FileBuffer()
{

}

FileBuffer::~FileBuffer()
{

}

bool FileBuffer::LoadFromFile(const char *fileName)
{
	FILE *srcFile = fopen(fileName, "rb");
	if (srcFile == NULL)
		return false;

	fseek(srcFile, 0, SEEK_END);
	size_t fileSize = ftell(srcFile);
	fseek(srcFile, 0, SEEK_SET);

	void *p = GetBuffer(fileSize);
	if (p == NULL)
	{
		fclose(srcFile);
		return false;
	}

	fread(p, 1, fileSize, srcFile);
	fclose(srcFile);
	return true;
}

bool FileBuffer::LoadFromPrivateFile(const char *fileName)
{
	FILE *srcFile = fopen(fileName, "rb");
	if (srcFile == NULL)
		return false;

	fseek(srcFile, 0, SEEK_END);
	size_t fileSize = ftell(srcFile);
	fseek(srcFile, 0, SEEK_SET);

	// 判断是不是我们专用的文件格式，是就需要对文件格式进行解析
	if (fileSize >= sizeof(FILE_HEADER))
	{
		uint32_t fileSign;
		fread(&fileSign, 1, sizeof(fileSign), srcFile);
		fseek(srcFile, 0, SEEK_SET);

		if (fileSign == FILE_SIGN)
			return DecodePrivateFile(srcFile, fileSize);
	}

	// 加载普通文件
	void *p = GetBuffer(fileSize);
	if (p == NULL)
	{
		fclose(srcFile);
		Empty();
		return false;
	}

	fread(p, 1, fileSize, srcFile);
	fclose(srcFile);
	return true;
}

bool FileBuffer::DecodePrivateFile(FILE *srcFile, size_t fileSize)
{
	FILE_HEADER fileHeader;
	fread(&fileHeader, 1, sizeof(fileHeader), srcFile);

	// 检查读出来的文件大小与记录中的大小是否有差
	if (fileHeader.DataSize > fileSize - sizeof(FILE_HEADER))
	{
		fclose(srcFile);
		return false;
	}

	void *p = GetBuffer(fileHeader.DataSize);
	if (p == NULL)
	{
		fclose(srcFile);
		return false;
	}

	fread(p, 1, fileHeader.DataSize, srcFile);
	fclose(srcFile);

	// 检验的数据CRC是否正确
	if (do_crc32(p, fileHeader.DataSize) != fileHeader.DataCrc)
	{
		Empty();
		return false;
	}

	// 判断是否需要解压缩
	if (fileHeader.Compressed)
	{
		CLz77 lz77 = CLz77::Decompress(GetBuffer(), GetSize());
		// 检查解压后的数据大小与实际大小是否相同
		if (lz77.GetSize() != fileHeader.OriginalDataSize)
		{
			Empty();
			return false;
		}

		// 将缓冲区重新指向已解压的数据
		CopyOf(lz77);
	}
	return true;
}

bool FileBuffer::SaveToFile(const char *fileName)
{
	FILE *dstFile = fopen(fileName, "wb");
	if (dstFile == NULL)
		return false;

	size_t writtenSize = fwrite(GetBuffer(), 1, GetSize(), dstFile);
	fclose(dstFile);
	return writtenSize == GetSize();
}

bool FileBuffer::SaveToPrivateFile(const char *fileName, bool compress)
{
	CBuffer fileDataBuffer = *this;
	FILE_HEADER fileHeader = {0};
	fileHeader.FileSign = FILE_SIGN;
	fileHeader.DataSize = GetSize();
	fileHeader.OriginalDataSize = GetSize();

	if (compress)
	{
		CLz77 lz77 = CLz77::Compress(GetBuffer(), GetSize(), 2);
		if (lz77.IsEmpty())
			return false;

		fileHeader.Compressed = 1;              // 标识数据被压缩过
		fileHeader.DataSize = lz77.GetSize();   // 数据长度改为压缩后的长度
		fileDataBuffer = lz77;
	}

	// 计算并保存数据的CRC值
	fileHeader.DataCrc = do_crc32(fileDataBuffer.GetBuffer(), fileHeader.DataSize);

	FILE *dstFile = fopen(fileName, "wb");
	if (dstFile == NULL)
		return false;

	// 写入文件头
	size_t writtenSize = fwrite(&fileHeader, 1, sizeof(fileHeader), dstFile);
	if (writtenSize != sizeof(fileHeader))
	{
		fclose(dstFile);
		return false;
	}

	// 写入已压缩数据
	writtenSize = fwrite(fileDataBuffer.GetBuffer(), 1, fileHeader.DataSize, dstFile);
	fclose(dstFile);
	return writtenSize == fileHeader.DataSize;
}

bool FileBuffer::ConvertToUtf8()
{
	if (IsEmpty())
		return false;

	const char *text = (const char *)GetBuffer();
	size_t textLen = GetSize();
	if (textLen >= 3 && strncmp(text, "\xef\xbb\xbf", 3) == 0)
	{
		// 带BOM签名的UTF8文件，移除BOM
		Delete(0, 3);
		return true;
	}
	if (textLen >= 2 && strncmp(text, "\xff\xfe", 2) == 0)
	{
		// Unicode文件，转码成Utf8格式
		CBuffer tmp;
		void *p = tmp.GetBuffer(GetSize() * 3 + 3);
		if (!p)
			return false;

		int len = Ucs2ToUtf8((uint16_t *)GetBuffer() + 1, -1, (char *)p, tmp.GetSize());
		tmp.ReleaseBuffer(len);
		CopyOf(tmp);
		return true;
	}
	return true;
}

uint32_t FileBuffer::CalculateCrc32()
{
	if (IsEmpty())
		return 0;
	return do_crc32(GetBuffer(), GetSize());
}

bool WriteDataToFile(const char *fileName, const void *data, size_t dataSize, bool append)
{
	FILE *file = fopen(fileName, append ? "wb+" : "wb");
	if (file == NULL)
		return false;

	size_t writtenSize = fwrite(data, 1, dataSize, file);
	fclose(file);
	return writtenSize == dataSize;
}

bool CalculateFileCrc32(const char *fileName, uint32_t *fileCrcValue)
{
	FileBuffer fb;
	if (!fb.LoadFromFile(fileName))
		return false;

	*fileCrcValue = do_crc32(fb.GetBuffer(), fb.GetSize());
	return true;
}
