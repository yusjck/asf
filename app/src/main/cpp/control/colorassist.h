#ifndef _COLORASSIST_H
#define _COLORASSIST_H

#include <set>
#include "buffer.h"
#include "object.h"
#include "handle.h"
#include "deviceinfo.h"
#include "simpleocr.h"

class MemoryBitmap : public CRefObject
{
public:
	int CaptureScreen(int x, int y, int width, int height, DeviceInfo *deviceInfo);
	int LoadFromPngFile(const void *imgFile, uint32_t imgFileSize);
	int CopyBitmap(int x, int y, int width, int height, CAutoRef<MemoryBitmap> &outputBitmap);
	int SaveToPngFile(CBuffer &file);
	void ReleaseBitmap();

	int GetX() { return m_x; }
	int GetY() { return m_y; }
	int GetWidth() { return m_width; }
	int GetHeight() { return m_height; }
	uint8_t *GetBits() { return m_bits; }

	MemoryBitmap();
	~MemoryBitmap();

private:
	int m_x, m_y;
	int m_width, m_height;
	uint8_t *m_bits;
};

class ColorAssist
{
public:
	int CaptureBitmap(int x, int y, int width, int height, handle_t &bitmapHandle);
	int ReleaseBitmap(handle_t bitmapHandle);
	int CreateBitmap(const void *imgFile, uint32_t imgFileSize, handle_t &bitmapHandle);
	int GetPreviousCapture(handle_t &bitmapHandle);
	int CaptureBitmapToPngFile(handle_t bitmapHandle, int x, int y, int width, int height, CBuffer &file);
	int FindPicture(handle_t bitmapHandle, int x, int y, int width, int height, handle_t sourceBitmapHandle, const char *colorMaskString, float similar, int maxResult, std::string &strResult);
	int FindColor(handle_t bitmapHandle, int x, int y, int width, int height, const char *colorString, float similar, int maxResult, std::string &strResult);
	int FindColorEx(handle_t bitmapHandle, int x, int y, int width, int height, const char *colorGroup, float similar, int type, int maxResult, std::string &strResult);
	int GetPixelColor(handle_t bitmapHandle, int x, int y, std::string &strColor);
	int GetColorCount(handle_t bitmapHandle, int x, int y, int width, int height, const char *colorString, float similar, int &colorCount);
	int IsSimilarColor(handle_t bitmapHandle, int x, int y, const char *colorString, float similar, bool &isSimilar);

	ColorAssist();
	~ColorAssist();

public:
	std::shared_ptr<DeviceInfo> m_deviceInfo;
	SimpleOcr m_simpleOcr;

private:
	HandleStore<MemoryBitmap> m_handleStore;
	handle_t m_previousCapture;

	friend class SimpleOcr;

	int GetBitmapStream(handle_t bitmapHandle, int &x, int &y, int &width, int &height, CAutoRef<MemoryBitmap> &memoryBitmap);
	void SetPreviousCapture(MemoryBitmap *memoryBitmap);
};

#endif
