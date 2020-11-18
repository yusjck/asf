#include "common.h"
#include <png.h>
#include "colorutils.h"
#include "colorassist.h"

typedef struct _READ_DATA {
	uint8_t *buf;
	uint32_t pointer;
	uint32_t length;
} READ_DATA;

static void PNGAPI user_read_data(png_structp png_ptr, png_bytep data, png_size_t length)
{
	READ_DATA *read_data = (READ_DATA *)png_get_io_ptr(png_ptr);
	if (read_data->pointer + length > read_data->length)
		png_error(png_ptr, "Read Error");
	memcpy(data, read_data->buf + read_data->pointer, length);
	read_data->pointer += length;
}

static void PNGAPI user_write_data(png_structp png_ptr, png_bytep data, png_size_t length)
{
	CBuffer *buf = (CBuffer *)png_get_io_ptr(png_ptr);
	buf->Write(data, length);
}

static int decompress_from_png(uint8_t *data, uint32_t size, CBuffer &buf, int &w, int &h)
{
	png_structp png_ptr;
	png_infop info_ptr;
	png_bytep *row_pointers;
	int x, y, color_type;

	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png_ptr)
	{
		LOGI("png_create_read_struct failed\n");
		return -1;
	}

	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr)
	{
		png_destroy_read_struct(&png_ptr, NULL, NULL);
		LOGI("png_create_info_struct failed\n");
		return -1;
	}

	if (setjmp(png_jmpbuf(png_ptr)))
	{
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
		LOGI("decompress_from_png failed\n");
		return -1;
	}

	READ_DATA read_data = {data, 0, size};
	png_set_read_fn(png_ptr, &read_data, user_read_data);
	png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_EXPAND, 0);

	color_type = png_get_color_type(png_ptr, info_ptr);
	w = png_get_image_width(png_ptr, info_ptr);
	h = png_get_image_height(png_ptr, info_ptr);
	row_pointers = png_get_rows(png_ptr, info_ptr);

	switch (color_type)
	{
	case PNG_COLOR_TYPE_RGB_ALPHA:
		for (y = 0; y < h; ++y)
		{
			for (x = 0; x < w * 4;)
			{
				uint8_t color[4] = {0};
				color[0] = row_pointers[y][x++];
				color[1] = row_pointers[y][x++];
				color[2] = row_pointers[y][x++];
				color[3] = row_pointers[y][x++];
				buf.Write(color, 4);
			}
		}
		break;

	case PNG_COLOR_TYPE_RGB:
		for (y = 0; y < h; ++y)
		{
			for (x = 0; x < w * 3;)
			{
				uint8_t color[4] = {0};
				color[0] = row_pointers[y][x++];
				color[1] = row_pointers[y][x++];
				color[2] = row_pointers[y][x++];
				buf.Write(color, 4);
			}
		}
		break;
	default:
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
		return -1;
	}
	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
	return 0;
}

static int compress_to_png(uint8_t *rgba, int width, int height, CBuffer &buf)
{
	png_structp png_ptr;
	png_infop info_ptr;
	png_bytep *row_pointers;

	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png_ptr)
	{
		LOGI("png_create_write_struct failed\n");
		return -1;
	}

	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr)
	{
		png_destroy_write_struct(&png_ptr, NULL);
		LOGI("png_create_info_struct failed\n");
		return -1;
	}

	if (setjmp(png_jmpbuf(png_ptr)))
	{
		png_destroy_write_struct(&png_ptr, &info_ptr);
		LOGI("compress_to_png failed\n");
		return -1;
	}

	png_set_write_fn(png_ptr, &buf, user_write_data, NULL);
	png_set_IHDR(png_ptr, info_ptr, width, height, 8, PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE,
			PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
	png_write_info(png_ptr, info_ptr);

	row_pointers = (png_bytep *)malloc(height * sizeof(png_bytep));
	for (int i = 0; i < height; i++)
	{
		row_pointers[i] = rgba + width * i * 4;
	}
	png_write_image(png_ptr, row_pointers);
	free(row_pointers);
	png_write_end(png_ptr, info_ptr);
	png_destroy_write_struct(&png_ptr, &info_ptr);
	return 0;
}

MemoryBitmap::MemoryBitmap()
{
	m_x = m_y = 0;
	m_width = m_height = 0;
	m_bits = NULL;
}

MemoryBitmap::~MemoryBitmap()
{
	ReleaseBitmap();
}

int MemoryBitmap::CaptureScreen(int x, int y, int width, int height, DeviceInfo *deviceInfo)
{
	ReleaseBitmap();

	// 检查截取区域是否合法
	if (width < 0 || height < 0 || x >= (x + width) || y >= (y + height))
		return ERR_INVALID_PARAMETER;

	m_bits = new(std::nothrow) uint8_t[width * height * 4];
	if (!m_bits)
		return ERR_INVOKE_FAILED;

	m_x = x;
	m_y = y;
	m_width = width;
	m_height = height;

	int ret = deviceInfo->ScreenShot(x, y, width, height, m_bits);
	if (!IS_SUCCESS(ret))
	{
		ReleaseBitmap();
		return ret;
	}

	return ERR_NONE;
}

int MemoryBitmap::LoadFromPngFile(const void *imgFile, uint32_t imgFileSize)
{
	ReleaseBitmap();

	CBuffer buf;
	int width, height;
	if (decompress_from_png((uint8_t *)imgFile, imgFileSize, buf, width, height) == -1)
		return ERR_INVOKE_FAILED;

	size_t bitsSize = width * height * 4;
	m_bits = new(std::nothrow) uint8_t[bitsSize];
	if (!m_bits)
		return ERR_INVOKE_FAILED;

	m_x = 0;
	m_y = 0;
	m_width = width;
	m_height = height;
	memcpy(m_bits, buf.GetBuffer(), bitsSize);

	return ERR_NONE;
}

int MemoryBitmap::CopyBitmap(int x, int y, int width, int height, CAutoRef<MemoryBitmap> &outputBitmap)
{
	if (m_bits == NULL)
		return ERR_INVALID_INVOKE;

	// 先判断坐标和宽高是否都是0，都是0代表使用图片原始的坐标和尺寸，不是非法参数
	if ((x || y || width || height) && (width < 0 || height < 0 || x >= (x + width) || y >= (y + height)))
		return ERR_INVALID_PARAMETER;

	if (width == 0 && height == 0)
	{
		x = m_x;
		y = m_y;
		width = m_width;
		height = m_height;
	}

	CAutoRef<MemoryBitmap> newMemoryBitmap = new MemoryBitmap();
	newMemoryBitmap->m_x = x;
	newMemoryBitmap->m_y = y;
	newMemoryBitmap->m_width = width;
	newMemoryBitmap->m_height = height;

	size_t bitsSize = width * height * 4;
	newMemoryBitmap->m_bits = new(std::nothrow) uint8_t[bitsSize];
	if (!newMemoryBitmap->m_bits)
		return ERR_INVOKE_FAILED;

	try
	{
		uint8_t *bits = newMemoryBitmap->m_bits;
		memset(bits, 0, bitsSize);

		int startX = max(x, m_x);
		int startY = max(y, m_y);
		int endX = min(x + width, m_x + m_width);
		int endY = min(y + height, m_y + m_height);

		for (int yy = startY; yy < endY; yy++)
		{
			for (int xx = startX; xx < endX; xx++)
			{
				int y1 = yy - m_y;
				int x1 = xx - m_x;
				int y2 = yy - y;
				int x2 = xx - x;
				((uint32_t *)bits)[y2 * width + x2] = ((uint32_t *)m_bits)[y1 * m_width + x1];
			}
		}

		outputBitmap = newMemoryBitmap;
		return ERR_NONE;
	}
	catch (...)
	{
	}

	return ERR_INVOKE_FAILED;
}

int MemoryBitmap::SaveToPngFile(CBuffer &file)
{
	if (m_bits == NULL)
		return ERR_INVALID_INVOKE;

	if (compress_to_png(m_bits, m_width, m_height, file) != 0)
		return ERR_INVOKE_FAILED;

	return ERR_NONE;
}

void MemoryBitmap::ReleaseBitmap()
{
	m_x = m_y = 0;
	m_width = m_height = 0;
	if (m_bits)
	{
		delete [] m_bits;
		m_bits = NULL;
	}
}

ColorAssist::ColorAssist() : m_simpleOcr(this)
{
	m_previousCapture = NULL;
}

ColorAssist::~ColorAssist()
{

}

int ColorAssist::CaptureBitmap(int x, int y, int width, int height, handle_t &bitmapHandle)
{
	CAutoRef<MemoryBitmap> bmp = new MemoryBitmap();
	int ret = bmp->CaptureScreen(x, y, width, height, m_deviceInfo.get());
	if (!IS_SUCCESS(ret))
		return ret;

	bitmapHandle = m_handleStore.Insert(bmp);
	return ERR_NONE;
}

int ColorAssist::ReleaseBitmap(handle_t bitmapHandle)
{
	if (m_handleStore.Delete(bitmapHandle))
		return ERR_NONE;

	return ERR_INVALID_PARAMETER;
}

int ColorAssist::CreateBitmap(const void *imgFile, uint32_t imgFileSize, handle_t &bitmapHandle)
{
	CAutoRef<MemoryBitmap> bmp = new MemoryBitmap();
	int ret = bmp->LoadFromPngFile(imgFile, imgFileSize);
	if (!IS_SUCCESS(ret))
		return ret;

	bitmapHandle = m_handleStore.Insert(bmp);
	return ERR_NONE;
}

int ColorAssist::GetPreviousCapture(handle_t &bitmapHandle)
{
	if (!m_previousCapture)
		return ERR_NOT_EXIST;

	bitmapHandle = m_previousCapture;
	return ERR_NONE;
}

void ColorAssist::SetPreviousCapture(MemoryBitmap *memoryBitmap)
{
	// 释放上一次截取的屏幕位图
	if (m_previousCapture)
	{
		m_handleStore.Delete(m_previousCapture);
		m_previousCapture = NULL;
	}

	m_previousCapture = m_handleStore.Insert(memoryBitmap);
}

int ColorAssist::GetBitmapStream(handle_t bitmapHandle, int &x, int &y, int &width, int &height, CAutoRef<MemoryBitmap> &memoryBitmap)
{
	CAutoRef<MemoryBitmap> bmp = NULL;
	if (bitmapHandle == NULL)
	{
		// bitmapHandle为NULL表示要从当前屏幕上抓取最新图像
		bmp = new MemoryBitmap();
		int ret = bmp->CaptureScreen(x, y, width, height, m_deviceInfo.get());
		if (!IS_SUCCESS(ret))
			return ret;
	}
	else
	{
		// 判断传入的bitmapHandle是否有效
		CAutoRef<MemoryBitmap> existsBmp = m_handleStore.Query(bitmapHandle);
		if (!existsBmp.Get())
			return ERR_INVALID_PARAMETER;

		int ret = existsBmp->CopyBitmap(x, y, width, height, bmp);
		if (!IS_SUCCESS(ret))
			return ret;
	}

	SetPreviousCapture(bmp);

	x = bmp->GetX();
	y = bmp->GetY();
	width = bmp->GetWidth();
	height = bmp->GetHeight();
	memoryBitmap = bmp;
	return ERR_NONE;
}

int ColorAssist::CaptureBitmapToPngFile(handle_t bitmapHandle, int x, int y, int width, int height, CBuffer &file)
{
	CAutoRef<MemoryBitmap> memoryBitmap = NULL;
	int ret = GetBitmapStream(bitmapHandle, x, y, width, height, memoryBitmap);
	if (!IS_SUCCESS(ret))
		return ret;

	return memoryBitmap->SaveToPngFile(file);
}

static bool CompareColor(uint32_t color1, uint32_t color2, int iSimilar)
{
	if (iSimilar == 0)
	{
		if ((*(RGBQUAD *)&color1).rgbBlue == (*(RGBQUAD *)&color2).rgbBlue &&
		    (*(RGBQUAD *)&color1).rgbGreen == (*(RGBQUAD *)&color2).rgbGreen &&
		    (*(RGBQUAD *)&color1).rgbRed == (*(RGBQUAD *)&color2).rgbRed)
			return true;
	}
	else
	{
		if (abs((*(RGBQUAD *)&color1).rgbBlue - (*(RGBQUAD *)&color2).rgbBlue) <= iSimilar &&
		    abs((*(RGBQUAD *)&color1).rgbGreen - (*(RGBQUAD *)&color2).rgbGreen) <= iSimilar &&
		    abs((*(RGBQUAD *)&color1).rgbRed - (*(RGBQUAD *)&color2).rgbRed) <= iSimilar)
			return true;
	}
	return false;
}

int ColorAssist::FindPicture(handle_t bitmapHandle, int x, int y, int width, int height, handle_t sourceBitmapHandle, const char *colorMaskString, float similar, int maxResult, std::string &strResult)
{
	ColorParser colorMask;
	if (colorMaskString && !colorMask.ParseColorString(colorMaskString, 1))
		return ERR_INVALID_PARAMETER;

	CAutoRef<MemoryBitmap> memoryBitmap = NULL;
	int ret = GetBitmapStream(bitmapHandle, x, y, width, height, memoryBitmap);
	if (!IS_SUCCESS(ret))
		return ret;

	int sourceX = 0, sourceY = 0, sourceWidth = 0, sourceHeight = 0;
	CAutoRef<MemoryBitmap> sourceMemoryBitmap = NULL;
	ret = GetBitmapStream(sourceBitmapHandle, sourceX, sourceY, sourceWidth, sourceHeight, sourceMemoryBitmap);
	if (!IS_SUCCESS(ret))
		return ret;

	uint32_t *pixels = (uint32_t *)memoryBitmap->GetBits();
	uint32_t *sourcePixels = (uint32_t *)sourceMemoryBitmap->GetBits();
	// 找图片中前三个有效像素点的坐标
	int colorPixelX[3], colorPixelY[3];
	int colorPixelCount = 0;
	uint32_t pixelColor[3];
	for (int y1 = 0; y1 < sourceHeight; y1++)
	{
		for (int x1 = 0; x1 < sourceWidth; x1++)
		{
			uint32_t sourceColor = sourcePixels[y1 * sourceWidth + x1];
			// 忽略源图中的定义为透明色的颜色点
			if (colorMaskString != NULL && colorMask.IsSimilarColor(*(RGBQUAD *)&sourceColor))
				continue;
			// 避免取到临近且颜色相同的点
			if (colorPixelCount > 0 && CompareColor(sourceColor, pixelColor[colorPixelCount - 1], 32))
				continue;
			colorPixelX[colorPixelCount] = x1;
			colorPixelY[colorPixelCount] = y1;
			pixelColor[colorPixelCount] = sourceColor;
			if (++colorPixelCount >= 3)
				goto GetPixelFinished;
		}
	}

GetPixelFinished:
	int iSimilar;
	// 相似度大于1的时候，把它当做单色上下差的值
	if (similar > 1)
		iSimilar = (int)similar;
	else
		iSimilar = (int)(255 * (1 - similar));

	strResult = "";
	if (maxResult > 0 && colorPixelCount > 0)
	{
		for (int y1 = 0; y1 <= height - sourceHeight; y1++)
		{
			for (int x1 = 0; x1 <= width - sourceWidth; x1++)
			{
				// 比较预取的三个点的颜色是否和目标图片一致，是的话再进入全图匹配
				switch (colorPixelCount)
				{
				case 3:
					if (!CompareColor(pixels[(y1 + colorPixelY[2]) * width + (x1 + colorPixelX[2])], pixelColor[2], iSimilar))
						continue;
					break;
				case 2:
					if (!CompareColor(pixels[(y1 + colorPixelY[1]) * width + (x1 + colorPixelX[1])], pixelColor[1], iSimilar))
						continue;
					break;
				case 1:
					if (!CompareColor(pixels[(y1 + colorPixelY[0]) * width + (x1 + colorPixelX[0])], pixelColor[0], iSimilar))
						continue;
					break;
				default:
					assert(false);
				}

				// 进行全图匹配
				for (int y2 = 0; y2 < sourceHeight; y2++)
				{
					for (int x2 = 0; x2 < sourceWidth; x2++)
					{
						uint32_t sourceColor = sourcePixels[y2 * sourceWidth + x2];
						if (colorMaskString != NULL && colorMask.IsSimilarColor(*(RGBQUAD *)&sourceColor))
							continue;
						if (!CompareColor(pixels[(y1 + y2) * width + (x1 + x2)], sourceColor, iSimilar))
							goto NotFound;
					}
				}

				// 记录下图片的相对坐标
				if (strResult.empty())
				{
					char fmt[30];
					sprintf(fmt, "%d|%d", x + x1, y + y1);
					strResult += fmt;
				}
				else
				{
					char fmt[30];
					sprintf(fmt, ",%d|%d", x + x1, y + y1);
					strResult += fmt;
				}

				if (--maxResult == 0)
					goto FindEnd;
NotFound:;
			}
		}
	}

FindEnd:
	return ERR_NONE;
}

int ColorAssist::FindColor(handle_t bitmapHandle, int x, int y, int width, int height, const char *colorString, float similar, int maxResult, std::string &strResult)
{
	ColorParser colorParser;
	if (!colorParser.ParseColorString(colorString, similar))
		return ERR_INVALID_PARAMETER;

	CAutoRef<MemoryBitmap> memoryBitmap = NULL;
	int ret = GetBitmapStream(bitmapHandle, x, y, width, height, memoryBitmap);
	if (!IS_SUCCESS(ret))
		return ret;

	uint32_t *pixels = (uint32_t *)memoryBitmap->GetBits();
	strResult = "";
	if (maxResult > 0)
	{
		// 查找相同或相似的颜色数量
		for (int y1 = 0; y1 < height; y1++)
		{
			for (int x1 = 0; x1 < width; x1++)
			{
				if (!colorParser.IsSimilarColor(*(RGBQUAD *)&pixels[y1 * width + x1]))
					continue;

				// 记录下已找到点的相对坐标
				if (strResult.empty())
				{
					char fmt[30];
					sprintf(fmt, "%d|%d", x + x1, y + y1);
					strResult += fmt;
				}
				else
				{
					char fmt[30];
					sprintf(fmt, ",%d|%d", x + x1, y + y1);
					strResult += fmt;
				}

				if (--maxResult == 0)
					goto FindEnd;
			}
		}
	}

FindEnd:
	return ERR_NONE;
}

static bool FindColor1(uint32_t *pixels, int width, int height, ColorParserEx &colorParser, int *foundX, int *foundY)
{
	if (*foundX == -1 && *foundY == -1)
	{
		*foundX = 0;
		*foundY = 0;
	}
	else
	{
		*foundX += 1;
	}
	int y = *foundY;
	int x = *foundX;
	for (; y < height; y++)
	{
		for (; x < width; x++)
		{
			if (colorParser.IsSimilarColors((RGBQUAD *)pixels, x, y, width, height))
			{
				*foundX = x;
				*foundY = y;
				return true;
			}
		}
		x = 0;
	}
	return false;
}

static bool FindColor2(uint32_t *pixels, int width, int height, ColorParserEx &colorParser, int *foundX, int *foundY)
{
	if (*foundX == -1 && *foundY == -1)
	{
		*foundX = width - 1;
		*foundY = height - 1;
	}
	else
	{
		*foundX -= 1;
	}
	int y = *foundY;
	int x = *foundX;
	for (; y >= 0; y--)
	{
		for (; x >=0; x--)
		{
			if (colorParser.IsSimilarColors((RGBQUAD *)pixels, x, y, width, height))
			{
				*foundX = x;
				*foundY = y;
				return true;
			}
		}
		x = width - 1;
	}
	return false;
}

static bool FindColor3(uint32_t *pixels, int width, int height, ColorParserEx &colorParser, int *foundX, int *foundY)
{
	int lx = width / 2, ly = height / 2;
	int rx = lx + 1, ry = ly + 1;
	int x = -1, y = -1;
	bool isFound = false;

	while (lx >= 0 || ly >= 0 || rx < width || ry < height)
	{
		if (lx < 0) lx = 0;
		if (ly < 0) ly = 0;
		if (rx >= width) rx = width - 1;
		if (ry >= height) ry = height - 1;
		for (x = lx; x < rx; x++)
		{
			if (colorParser.IsSimilarColors((RGBQUAD *)pixels, x, ly, width, height))
			{
				y = ly;
				isFound = true;
				break;
			}
			if (colorParser.IsSimilarColors((RGBQUAD *)pixels, x, ry, width, height))
			{
				y = ry;
				isFound = true;
				break;
			}
		}
		if (isFound)
			break;
		for (y = ly + 1; y < ry - 1; y++)
		{
			if (colorParser.IsSimilarColors((RGBQUAD *)pixels, lx, y, width, height))
			{
				x = lx;
				isFound = true;
				break;
			}
			if (colorParser.IsSimilarColors((RGBQUAD *)pixels, rx, y, width, height))
			{
				x = rx;
				isFound = true;
				break;
			}
		}
		if (isFound)
			break;
		lx--;
		ly--;
		rx++;
		ry++;
	}

	if (isFound)
	{
		*foundX = x;
		*foundY = y;
		return true;
	}

	return false;
}

int ColorAssist::FindColorEx(handle_t bitmapHandle, int x, int y, int width, int height, const char *colorGroup, float similar, int type, int maxResult, std::string &strResult)
{
	ColorParserEx colorParser;
	if (!colorParser.ParseColorString(colorGroup, similar))
		return ERR_INVALID_PARAMETER;

	// 获取32位色的像素流
	CAutoRef<MemoryBitmap> memoryBitmap = NULL;
	int ret = GetBitmapStream(bitmapHandle, x, y, width, height, memoryBitmap);
	if (!IS_SUCCESS(ret))
		return ret;

	uint32_t *pixels = (uint32_t *)memoryBitmap->GetBits();
// 	int nPixelTotal = width * height;
// 	colorParser.SortColorArray((LPRGBQUAD)pixels, nPixelTotal);

	if (type == 3 && maxResult > 1)
		maxResult = 1;

	strResult = "";
	int foundX = -1, foundY = -1;
	for (int i = 0; i < maxResult; i++)
	{
		bool isFound = false;
		switch (type)
		{
		case 1:
			// 从左上往右下查找点
			isFound = FindColor1(pixels, width, height, colorParser, &foundX, &foundY);
			break;
		case 2:
			// 从右下往左上查找点
			isFound = FindColor2(pixels, width, height, colorParser, &foundX, &foundY);
			break;
		case 3:
			// 从中间往四周查找点
			isFound = FindColor3(pixels, width, height, colorParser, &foundX, &foundY);
			break;
		default:
			return ERR_INVALID_PARAMETER;
		}

		if (!isFound)
			break;

		if (strResult.empty())
		{
			char fmt[30];
			sprintf(fmt, "%d|%d", x + foundX, y + foundY);
			strResult += fmt;
		}
		else
		{
			char fmt[30];
			sprintf(fmt, ",%d|%d", x + foundX, y + foundY);
			strResult += fmt;
		}
	}

	return ERR_NONE;
}

int ColorAssist::GetPixelColor(handle_t bitmapHandle, int x, int y, std::string &strColor)
{
	int width = 1, height = 1;
	CAutoRef<MemoryBitmap> memoryBitmap = NULL;
	int ret = GetBitmapStream(bitmapHandle, x, y, width, height, memoryBitmap);
	if (!IS_SUCCESS(ret))
		return ret;

	uint32_t *pixels = (uint32_t *)memoryBitmap->GetBits();
	strColor = format("%02x%02x%02x", ((RGBQUAD *)pixels)->rgbBlue, ((RGBQUAD *)pixels)->rgbGreen, ((RGBQUAD *)pixels)->rgbRed);
	return ERR_NONE;
}

int ColorAssist::GetColorCount(handle_t bitmapHandle, int x, int y, int width, int height, const char *colorString, float similar, int &colorCount)
{
	ColorParser colorParser;
	if (!colorParser.ParseColorString(colorString, similar))
		return ERR_INVALID_PARAMETER;

	CAutoRef<MemoryBitmap> memoryBitmap = NULL;
	int ret = GetBitmapStream(bitmapHandle, x, y, width, height, memoryBitmap);
	if (!IS_SUCCESS(ret))
		return ret;

	uint32_t *pixels = (uint32_t *)memoryBitmap->GetBits();
	int pixelTotal = width * height;
	colorCount = 0;

	// 查找相同或相似的颜色数量
	for (int i = 0; i < pixelTotal; i++)
	{
		if (!colorParser.IsSimilarColor(*(RGBQUAD *)&pixels[i]))
			continue;
		colorCount++;
	}

	return ERR_NONE;
}

int ColorAssist::IsSimilarColor(handle_t bitmapHandle, int x, int y, const char *colorString, float similar, bool &isSimilar)
{
	ColorParser colorParser;
	if (!colorParser.ParseColorString(colorString, similar))
		return ERR_INVALID_PARAMETER;

	int width = 1, height = 1;
	CAutoRef<MemoryBitmap> memoryBitmap = NULL;
	int ret = GetBitmapStream(bitmapHandle, x, y, width, height, memoryBitmap);
	if (!IS_SUCCESS(ret))
		return ret;

	uint32_t *pixels = (uint32_t *)memoryBitmap->GetBits();
	uint32_t color = *pixels & 0xffffff;
	isSimilar = colorParser.IsSimilarColor(*(RGBQUAD *)&color);
	return ERR_NONE;
}
