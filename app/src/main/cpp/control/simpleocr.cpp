#include "common.h"
#include "base64.h"
#include "colorutils.h"
#include "colorassist.h"
#include "simpleocr.h"

typedef struct _FONT_BITS {
	struct _FONT_BITS *Next;
	uint32_t Length;
	uint32_t CharCodeIsOffset;
	uint32_t CharCode;
	uint16_t Width;
	uint16_t Height;
	uint16_t ThreePixelX[3];
	uint16_t ThreePixelY[3];
	uint32_t PixelCount;
	uint8_t Bits[];
} FONT_BITS, *PFONT_BITS;

typedef struct _OCR_FOUND_CHAR{
	const char *CharText;
	int FoundX;
	int FoundY;
	int FontHeight;
} OCR_FOUND_CHAR, *POCR_FOUND_CHAR;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

SimpleOcr::SimpleOcr(ColorAssist *colorAssist)
{
	m_colorAssist = colorAssist;
}

SimpleOcr::~SimpleOcr()
{
	// 删除字典
	OCR_DICTS::iterator iter = m_ocrDicts.begin();
	while (iter != m_ocrDicts.end())
	{
		ReleaseFontBits(iter->second);
		iter++;
	}
}

PFONT_BITS SimpleOcr::LoadFontBits(const char *fontData)
{
	PFONT_BITS fontBits = NULL;
	PFONT_BITS *ppFontBits = &fontBits;
	std::vector<std::string> d = split(fontData, "\r\n");
	for (size_t i = 0; i < d.size(); i++)
	{
		if (d[i].length() < 5)
			continue;
		// 从后往前查找分割符"="，查找前先将原串长度减去2，因为BASE64编码最多会在编码末尾生成两个=
		size_t npos = d[i].substr(0, d[i].length() - 2).rfind("=");
		if (npos == -1)
			continue;

		std::string strFontText = d[i].substr(0, npos);
		std::string strFontData = d[i].substr(npos + 1);

		CBase64 base64 = CBase64::Decode(strFontData.c_str());
		const uint8_t *inPtr = (const uint8_t *)base64.GetBuffer();
		if (base64.GetSize() < 3)
			continue;

		int width, height;
		if (*inPtr < 247)
			width = *inPtr++;
		else
		{
			width = (*inPtr++ - 247) * 256;
			width += *inPtr++;
		}
		if (*inPtr < 247)
			height = *inPtr++;
		else
		{
			height = (*inPtr++ - 247) * 256;
			height += *inPtr++;
		}

		if (width == 0 || height == 0)
			continue;

		if ((size_t)(width * height / 8 + 2) > base64.GetSize())
			continue;

		uint32_t fontBitsSize = sizeof(FONT_BITS) + width * height + (strFontText.length() + 1);
		PFONT_BITS font = (PFONT_BITS)new uint8_t[fontBitsSize];
		memset(font, 0, fontBitsSize);

		font->Length = fontBitsSize;
		font->CharCodeIsOffset = 1;
		font->CharCode = sizeof(FONT_BITS) + width * height;
		strcpy((char *)((uint8_t *)font + font->CharCode), strFontText.c_str());
		font->Width = width;
		font->Height = height;

		font->PixelCount = 0;
		int bitCount = 0;
		// 将点阵格式由位转成字节
		for (int i = 0; i < width * height; i++)
		{
			if ((*inPtr & (1 << bitCount)) != 0)
			{
				// 以0和1记录下字体点阵
				font->Bits[i] = 1;
				if (font->PixelCount < 3)
				{
					// 记录下字体中前三个像素点的坐标
					font->ThreePixelX[font->PixelCount] = i % font->Width;
					font->ThreePixelY[font->PixelCount] = i / font->Width;
				}
				font->PixelCount++;
			}
			else
			{
				font->Bits[i] = 0;
			}
			if (++bitCount > 7)
			{
				inPtr++;
				bitCount = 0;
			}
		}

		// 将所有字符点阵信息做成一个链表
		*ppFontBits = font;
		ppFontBits = &font->Next;
	}

	return fontBits;
}

PFONT_BITS SimpleOcr::GetNextFontBits(PFONT_BITS fontBits)
{
	return fontBits->Next;
}

void SimpleOcr::ReleaseFontBits(PFONT_BITS fontBits)
{
	while (fontBits)
	{
		PFONT_BITS font = fontBits;
		fontBits = font->Next;
		delete [] font;
	}
}

PFONT_BITS SimpleOcr::GetFontBits(const char *text, const char *dictName)
{
	OCR_DICTS::iterator iter = m_ocrDicts.find(dictName);
	if (iter == m_ocrDicts.end())
		return NULL;

	PFONT_BITS fontBits = NULL;
	PFONT_BITS *ppFontBits = &fontBits;
	while (*text)
	{
		PFONT_BITS font = iter->second;
		for (; font; font = GetNextFontBits(font))
		{
			const char *charText;
			if (font->CharCodeIsOffset)
				charText = (const char *)((uint8_t *)font + font->CharCode);
			else
				charText = (const char *)&(font->CharCode);
			if (strncmp(text, charText, strlen(charText)) == 0)
			{
				*ppFontBits = (PFONT_BITS)new uint8_t[font->Length];
				memcpy(*ppFontBits, font, font->Length);
				(*ppFontBits)->Next = NULL;
				ppFontBits = &(*ppFontBits)->Next;
				text += strlen(charText);
				break;
			}
		}
		if (font == NULL)
		{
			ReleaseFontBits(fontBits);
			return NULL;
		}
	}

	return fontBits;
}

class FontSearch
{
public:
	__inline uint32_t ScopeGetPixelColor(int x, int y)
	{
		return m_colorPixels[(m_scopeY + y) * m_imageWidth + (m_scopeX + x)];
	}

	bool ScopeSearchFont(PFONT_BITS font, int &x, int &y)
	{
		int fontWidth = font->Width, fontHeight = font->Height;

		if (x + fontWidth > m_scopeWidth || y + fontHeight > m_scopeHeight)
			return false;

		// 字体不存在任何像素，无法判断颜色，比如空格之类的，只能假设存在
		if (font->PixelCount == 0)
		{
			x = m_scopeX;
			y = m_scopeY;
			return true;
		}

		int x1 = x, y1 = y;
		for (; y1 <= m_scopeHeight - fontHeight; y1++)
		{
			for (; x1 <= m_scopeWidth - fontWidth; x1++)
			{
				uint32_t color = ScopeGetPixelColor(x1 + font->ThreePixelX[0], y1 + font->ThreePixelY[0]);
				// 比较字体前三个点的颜色，如果存在差异就不需要继续比较了
				switch (font->PixelCount)
				{
					default:
					case 3:
						if (color != ScopeGetPixelColor(x1 + font->ThreePixelX[2], y1 + font->ThreePixelY[2]))
							goto NotFind;
					case 2:
						if (color != ScopeGetPixelColor(x1 + font->ThreePixelX[1], y1 + font->ThreePixelY[1]))
							goto NotFind;
					case 1:
					case 0:
						break;
				}
				for (int y2 = 0; y2 < fontHeight; y2++)
				{
					for (int x2 = 0; x2 < fontWidth; x2++)
					{
						if (font->Bits[y2 * fontWidth + x2])
						{
							// 比较字体部分
							if (ScopeGetPixelColor(x1 + x2, y1 + y2) != color)
								goto NotFind;
						}
						else
						{
							// 比较背景部分
							if (ScopeGetPixelColor(x1 + x2, y1 + y2) == color)
								goto NotFind;
						}
					}
				}
				x = m_scopeX + x1;
				y = m_scopeY + y1;
				return true;
NotFind:;
			}
			x1 = 0;
		}
		return false;
	}

	void SetSearchScope(int x, int y, int width, int height)
	{
		if (x >= m_imageWidth)
			x = m_imageWidth - 1;
		if (y >= m_iImageHeight)
			y = m_iImageHeight - 1;

		if (width > m_imageWidth - x)
			width = m_imageWidth - x;
		if (height > m_iImageHeight - y)
			height = m_iImageHeight - y;

		m_scopeX = x;
		m_scopeY = y;
		m_scopeWidth = width;
		m_scopeHeight = height;
	}

public:
	FontSearch(uint32_t *imageBits, int width, int height)
	{
		m_colorPixels = imageBits;
		m_imageWidth = width;
		m_iImageHeight = height;
		m_scopeX = 0;
		m_scopeY = 0;
		m_scopeWidth = width;
		m_scopeHeight = height;
	}

	~FontSearch()
	{
	}

private:
	uint32_t *m_colorPixels;
	int m_imageWidth;
	int m_iImageHeight;
	int m_scopeX;
	int m_scopeY;
	int m_scopeWidth;
	int m_scopeHeight;
};

int SimpleOcr::FindString(handle_t bitmapHandle, int x, int y, int width, int height, const char *text, const char *dictName, int maxResult, std::string &strResult)
{
	if (text == NULL || dictName == NULL)
		return ERR_INVALID_PARAMETER;

	if (strlen(text) == 0)
		return ERR_NOT_FOUND;

	// 获取字体像素流
	PFONT_BITS fontBits = GetFontBits(text, dictName);
	if (fontBits == NULL)
		return ERR_INVOKE_FAILED;

	// 获取屏幕像素流
	CAutoRef<MemoryBitmap> memoryBitmap = NULL;
	int ret = m_colorAssist->GetBitmapStream(bitmapHandle, x, y, width, height, memoryBitmap);
	if (!IS_SUCCESS(ret))
	{
		ReleaseFontBits(fontBits);
		return ret;
	}

	uint32_t *pixels = (uint32_t *)memoryBitmap->GetBits();

	int fontWidthSum = 0, fontHeight = 0;
	for (PFONT_BITS font = fontBits; font; font = GetNextFontBits(font))
	{
		fontWidthSum += font->Width;
		fontHeight = max(font->Height, fontHeight);
	}

	strResult = "";
	FontSearch fontSearch(pixels, width, height);
	int firstCharX = 0, firstCharY = 0;
	for (int i = 0; i < maxResult;)
	{
		if (firstCharX + fontWidthSum >= width)
		{
			firstCharX = 0;
			firstCharY++;
		}
		// 设置首字符的搜索区域
		fontSearch.SetSearchScope(0, 0, width - fontWidthSum + fontBits[0].Width, height);
		// 全图搜索第一个字符，找到返回坐标到x和y变量中
		if (!fontSearch.ScopeSearchFont(&fontBits[0], firstCharX, firstCharY))
			break;
		// iOtherCharX指向第一个字符的末尾，并作为第二个字符的起始坐标
		int otherCharX = firstCharX + fontBits[0].Width;
		int otherCharY = firstCharY;
		PFONT_BITS font = fontBits;
		for (;;)
		{
			// 指向第二个及后面的字符
			font = GetNextFontBits(font);
			if (font == NULL)
			{
				if (strResult.empty())
				{
					char fmt[30];
					sprintf(fmt, "%d|%d", x + firstCharX, y + firstCharY);
					strResult += fmt;
				}
				else
				{
					char fmt[30];
					sprintf(fmt, ",%d|%d", x + firstCharX, y + firstCharY);
					strResult += fmt;
				}
				firstCharX += 1;
				i++;
				break;
			}
			// 设定第二个以及后面字符的搜索区域
			fontSearch.SetSearchScope(otherCharX, otherCharY, font->Width + font->Height / 2, font->Height);
			int tempX = 0, tempY = 0;
			if (!fontSearch.ScopeSearchFont(font, tempX, tempY))
			{
				// 后续字符没搜索到，在新的区域重新搜索第一个字符
				firstCharX += 1;
				break;
			}
			// 指向当前字符的末尾
			otherCharX = tempX + font->Width;
		}
	}

	ReleaseFontBits(fontBits);
	return ERR_NONE;
}

class FontSearchEx
{
public:
	bool InitImage(uint32_t *imageBits, int width, int height, const char *colorString)
	{
		ColorParser colorParser;
		if (!colorParser.ParseColorString(colorString, 1))
			return false;

		uint8_t *binaryPixels = (uint8_t *)m_binaryPixels.GetBuffer(width * height);
		if (binaryPixels == NULL)
			return false;

		PPOINTS pixelIndexes = (PPOINTS)m_pixelIndexes.GetBuffer(width * height * sizeof(POINTS));
		if (pixelIndexes == NULL)
			return false;

		// 对图像进行二值化处理，并计算出所有字体像素的位置
		int lastIndex = 0;
		for (int i = 0; i < width * height; i++)
		{
			if (colorParser.IsSimilarColor(*(RGBQUAD *)&imageBits[i]))
			{
				binaryPixels[i] = 1;
				POINTS point = {(short)(i % width), (short)(i / width)};
				do
				{
					pixelIndexes[lastIndex] = point;
				} while (++lastIndex <= i);
			}
			else
			{
				binaryPixels[i] = 0;
			}
		}
		for (int n = width * height; lastIndex < n; lastIndex++)
		{
			POINTS point = {-1, -1};
			pixelIndexes[lastIndex] = point;
		}

		m_imageWidth = width;
		m_imageHeight = height;
		m_scopeX = 0;
		m_scopeY = 0;
		m_scopeWidth = width;
		m_scopeHeight = height;
		return true;
	}

	__inline uint8_t GetPixelValue(int x, int y)
	{
		return ((uint8_t *)m_binaryPixels.GetBuffer())[y * m_imageWidth + x];
	}

	__inline void SetPixelValue(int x, int y, uint8_t color)
	{
		((uint8_t *)m_binaryPixels.GetBuffer())[y * m_imageWidth + x] = color;
	}

	__inline uint8_t ScopeGetPixelValue(int x, int y)
	{
		return ((uint8_t *)m_binaryPixels.GetBuffer())[(m_scopeY + y) * m_imageWidth + (m_scopeX + x)];
	}

	bool ScopeSearchFont(PFONT_BITS font, float similar, int &x, int &y)
	{
		int fontWidth = font->Width, fontHeight = font->Height;

		if (x + fontWidth > m_scopeWidth || y + fontHeight > m_scopeHeight)
			return false;

		int x1 = x, y1 = y;
		for (; y1 <= m_scopeHeight - fontHeight; y1++)
		{
			for (; x1 <= m_scopeWidth - fontWidth; x1++)
			{
				uint32_t maxUnmatch;
				switch (font->PixelCount)
				{
					default:
					case 3:
						if (!ScopeGetPixelValue(x1 + font->ThreePixelX[2], y1 + font->ThreePixelY[2]))
							goto NotFind;
					case 2:
						if (!ScopeGetPixelValue(x1 + font->ThreePixelX[1], y1 + font->ThreePixelY[1]))
							goto NotFind;
					case 1:
						if (!ScopeGetPixelValue(x1 + font->ThreePixelX[0], y1 + font->ThreePixelY[0]))
							goto NotFind;
					case 0:
						break;
				}
				maxUnmatch = (uint32_t)(fontWidth * fontHeight * (1 - similar));
				for (int y2 = 0; y2 < fontHeight; y2++)
				{
					for (int x2 = 0; x2 < fontWidth; x2++)
					{
						if (font->Bits[y2 * fontWidth + x2])
						{
							if (!ScopeGetPixelValue(x1 + x2, y1 + y2) && maxUnmatch-- == 0)
								goto NotFind;
						}
						else
						{
							if (ScopeGetPixelValue(x1 + x2, y1 + y2) && maxUnmatch-- == 0)
								goto NotFind;
						}
					}
				}
				x = m_scopeX + x1;
				y = m_scopeY + y1;
				return true;
NotFind:;
			}
			x1 = 0;
		}
		return false;
	}

	bool FullSearchFont(PFONT_BITS font, float similar, int &x, int &y, bool checkSpace)
	{
		int fontWidth = font->Width, fontHeight = font->Height;

		if (x + fontWidth > m_imageWidth || y + fontHeight > m_imageHeight)
			return false;

		for (int i = y * m_imageWidth + x; i < m_imageWidth * m_imageHeight; i++)
		{
			// 从索引表中获取图片中下一个黑点的位置
			POINTS index = ((PPOINTS)m_pixelIndexes.GetBuffer())[i];
			if (index.x == -1 && index.y == -1)
				return false;
			// 让i也指向下一个黑点
			i = index.y * m_imageWidth + index.x;
			// 将黑点坐标减去黑体第一个点的坐标，得到字体左上角坐标
			int x1 = index.x - font->ThreePixelX[0];
			int y1 = index.y - font->ThreePixelY[0];
			if (x1 < 0 || y1 < 0 || x1 > m_imageWidth - fontWidth || y1 > m_imageHeight - fontHeight)
				continue;

			if (y1 < y || (y1 == y && x1 < x))
				continue;

			uint32_t maxUnmatch;
			switch (font->PixelCount)
			{
				default:
				case 3:
					if (!GetPixelValue(x1 + font->ThreePixelX[2], y1 + font->ThreePixelY[2]))
						goto NotFind;
				case 2:
					if (!GetPixelValue(x1 + font->ThreePixelX[1], y1 + font->ThreePixelY[1]))
						goto NotFind;
				case 1:
				case 0:
					break;
			}
			maxUnmatch = (uint32_t)(fontWidth * fontHeight * (1 - similar));
			for (int y2 = 0; y2 < fontHeight; y2++)
			{
				for (int x2 = 0; x2 < fontWidth; x2++)
				{
					if (font->Bits[y2 * fontWidth + x2])
					{
						if (!GetPixelValue(x1 + x2, y1 + y2) && maxUnmatch-- == 0)
							goto NotFind;
					}
					else
					{
						if (GetPixelValue(x1 + x2, y1 + y2) && maxUnmatch-- == 0)
							goto NotFind;
					}
				}
			}
			if (checkSpace)
			{
				// 判断字体前后是否存在空白，避免把一个字的某一部分当成另一个字
				for (int y2 = 0; y2 < fontHeight; y2++)
				{
					if (x1 > 0)
					{
						if (GetPixelValue(x1 - 1, y1 + y2))
							goto NotFind;
					}
					if (x1 + fontWidth < m_scopeWidth)
					{
						if (GetPixelValue(x1 + fontWidth, y1 + y2))
							goto NotFind;
					}
				}
			}
			x = x1;
			y = y1;
			return true;
NotFind:;
		}

		return false;
	}

	bool ClearFontArea(int x, int y, int fontWidth, int fontHeight)
	{
		if (x + fontWidth > m_imageWidth || y + fontHeight > m_imageHeight)
			return false;

		uint8_t *binaryPixels = (uint8_t *)m_binaryPixels.GetBuffer();
		for (int y1 = 0; y1 < fontHeight; y1++)
		{
			for (int x1 = 0; x1 < fontWidth; x1++)
			{
				SetPixelValue(x + x1, y + y1, 0);
			}
		}
		PPOINTS pixelIndexes = (PPOINTS)m_pixelIndexes.GetBuffer();
		int lastIndex = 0;
		for (int i = 0; i < m_imageWidth * m_imageHeight; i++)
		{
			if (binaryPixels[i] != 0)
			{
				POINTS point = {(short)(i % m_imageWidth), (short)(i / m_imageWidth)};
				do
				{
					pixelIndexes[lastIndex] = point;
				} while (++lastIndex <= i);
			}
		}
		for (int n = m_imageWidth * m_imageHeight; lastIndex < n; lastIndex++)
		{
			POINTS point = {-1, -1};
			pixelIndexes[lastIndex] = point;
		}
		return true;
	}

	void SetSearchScope(int x, int y, int width, int height)
	{
		if (x >= m_imageWidth)
			x = m_imageWidth - 1;
		if (y >= m_imageHeight)
			y = m_imageHeight - 1;

		if (width > m_imageWidth - x)
			width = m_imageWidth - x;
		if (height > m_imageHeight - y)
			height = m_imageHeight - y;

		m_scopeX = x;
		m_scopeY = y;
		m_scopeWidth = width;
		m_scopeHeight = height;
	}

public:
	FontSearchEx()
	{
		m_imageWidth = 0;
		m_imageHeight = 0;
		m_scopeX = 0;
		m_scopeY = 0;
		m_scopeWidth = 0;
		m_scopeHeight = 0;
	}
	~FontSearchEx()
	{
	}

private:
	typedef struct _POINTS {
		short x;
		short y;
	} POINTS, *PPOINTS;

	CBuffer m_binaryPixels;
	CBuffer m_pixelIndexes;
	int m_imageWidth;
	int m_imageHeight;
	int m_scopeX;
	int m_scopeY;
	int m_scopeWidth;
	int m_scopeHeight;
};

int SimpleOcr::FindStringEx(handle_t bitmapHandle, int x, int y, int width, int height, const char *text, const char *dictName, const char *colorString, float similar, int maxResult, std::string &strResult)
{
	if (text == NULL || dictName == NULL || colorString == NULL)
		return ERR_INVALID_PARAMETER;

	if (strlen(text) == 0)
		return ERR_NOT_FOUND;

	// 获取字体像素流
	PFONT_BITS fontBits = GetFontBits(text, dictName);
	if (fontBits == NULL)
		return ERR_INVOKE_FAILED;

	// 获取屏幕像素流
	CAutoRef<MemoryBitmap> memoryBitmap = NULL;
	int ret = m_colorAssist->GetBitmapStream(bitmapHandle, x, y, width, height, memoryBitmap);
	if (!IS_SUCCESS(ret))
	{
		ReleaseFontBits(fontBits);
		return ret;
	}

	uint32_t *pixels = (uint32_t *)memoryBitmap->GetBits();
	FontSearchEx fontSearch;
	if (!fontSearch.InitImage(pixels, width, height, colorString))
	{
		ReleaseFontBits(fontBits);
		return ERR_INVOKE_FAILED;
	}

	int fontWidthSum = 0, fontHeight = 0;
	for (PFONT_BITS font = fontBits; font; font = GetNextFontBits(font))
	{
		fontWidthSum += font->Width;
		fontHeight = max(font->Height, fontHeight);
	}

	strResult = "";
	int firstCharX = 0, firstCharY = 0;
	// 全图搜索第一个字符，找到返回坐标到x和y变量中
	for (int i = 0; i < maxResult;)
	{
		if (!fontSearch.FullSearchFont(&fontBits[0], similar, firstCharX, firstCharY, false))
			break;
		// otherCharX指向第一个字符的末尾，并作为第二个字符的起始坐标
		int otherCharX = firstCharX + fontBits[0].Width;
		int otherCharY = firstCharY;
		PFONT_BITS font = fontBits;
		// 继续搜索其它字符，全部找到就返回第一个字符坐标
		for (;;)
		{
			// 指向第二个及后面的字符
			font = GetNextFontBits(font);
			if (font == NULL)
			{
				if (strResult.empty())
				{
					char fmt[30];
					sprintf(fmt, "%d|%d", x + firstCharX, y + firstCharY);
					strResult += fmt;
				}
				else
				{
					char fmt[30];
					sprintf(fmt, ",%d|%d", x + firstCharX, y + firstCharY);
					strResult += fmt;
				}
				firstCharX += fontBits[0].Width;
				i++;
				break;
			}
			// 设定第二个以及后面字符的搜索区域
			fontSearch.SetSearchScope(otherCharX, otherCharY, font->Width + font->Height / 2, font->Height);
			int tempX = 0, tempY = 0;
			if (!fontSearch.ScopeSearchFont(font, similar, tempX, tempY))
			{
				// 后续字符没搜索到，在新的区域重新搜索第一个字符
				firstCharX += fontBits[0].Width;
				break;
			}
			// 指向当前字符的末尾
			otherCharX = tempX + font->Width;
		}
	}

	ReleaseFontBits(fontBits);
	return ERR_NONE;
}

bool SimpleOcr::SaveFontBits(PFONT_BITS fontBits, std::string &strResult)
{
	strResult = "";
	for (PFONT_BITS font = fontBits; font != NULL; font = GetNextFontBits(font))
	{
		int binaryAreaWidth = font->Width, binaryAreaHeight = font->Height;
		uint8_t *binaryArea = new uint8_t[2 + 2 + binaryAreaWidth * binaryAreaHeight / 8 + 1];
		memset(binaryArea, 0, 2 + 2 + binaryAreaWidth * binaryAreaHeight / 8 + 1);

		uint8_t *outPtr = binaryArea;
		// 如果值小于247就只使用一个字节来记录
		if (binaryAreaWidth < 247)
			*outPtr++ = (uint8_t)binaryAreaWidth;
		else
		{
			// 当值大于247时使用两个字节来记录该值，记录范围为0至(255-247)*256=2047
			*outPtr++ = 247 + (uint8_t)(binaryAreaWidth / 256);
			*outPtr++ = (uint8_t)(binaryAreaWidth % 256);
		}
		if (binaryAreaHeight < 247)
			*outPtr++ = (uint8_t)binaryAreaHeight;
		else
		{
			*outPtr++ = 247 + (uint8_t)(binaryAreaHeight / 256);
			*outPtr++ = (uint8_t)(binaryAreaHeight % 256);
		}

		int bitCount = 0;
		for (int y = 0; y < binaryAreaHeight; y++)
		{
			for (int x = 0; x < binaryAreaWidth; x++)
			{
				if (font->Bits[y * binaryAreaWidth + x] == 1)
					*outPtr |= 1 << bitCount;
				if (++bitCount > 7)
				{
					outPtr++;
					bitCount = 0;
				}
			}
		}
		if (bitCount)
			outPtr++;

		CBase64 base64 = CBase64::Encode(binaryArea, outPtr - binaryArea);
		std::string strLine;
		if (font->CharCodeIsOffset)
			strLine = (const char *)((uint8_t *)font + font->CharCode);
		else
			strLine = (const char *)&(font->CharCode);
		strResult += strLine + "=" + (const char *)base64.GetBuffer() + "\r\n";
		delete [] binaryArea;
	}
	return true;
}

int SimpleOcr::SetOcrDict(const char *dictName, int dictType, const char *dict)
{
	if (dictName == NULL || dict == NULL)
		return ERR_INVALID_PARAMETER;

//	if (dictType == 4)
//	{
//		OCR_DICTS::iterator iter = m_ocrDicts.find(dictName);
//		if (iter == m_ocrDicts.end())
//			return ERR_NAME_NOT_EXIST;
//
//		PFONT_BITS fontBits = iter->second;
//		std::string strResult;
//		if (!SaveFontBits(fontBits, strResult))
//			return ERR_INVOKE_FAILED;
//
//		if (!WriteUnicodeFile(dict, strResult.c_str()))
//			return ERR_SAVE_FILE_FAILED;
//
//		return ERR_NONE;
//	}

	OCR_DICTS::iterator iter = m_ocrDicts.find(dictName);
	if (iter != m_ocrDicts.end())
	{
		// 移除同名字典
		ReleaseFontBits(iter->second);
		m_ocrDicts.erase(iter);
	}

	if (dictType == 2)
	{
		PFONT_BITS fontBits = LoadFontBits(dict);
		if (fontBits == NULL)
			return ERR_INVOKE_FAILED;

		// 添加到字典集中
		m_ocrDicts.insert(std::make_pair(dictName, fontBits));
		return ERR_NONE;
	}

	return ERR_INVALID_PARAMETER;
}

int SimpleOcr::OcrExtract(handle_t bitmapHandle, int x, int y, int width, int height, const char *colorString, std::string &strResult)
{
	if (width > 2047 || height > 2047)
		return ERR_INVALID_PARAMETER;

	ColorParser colorParser;
	if (!colorParser.ParseColorString(colorString, 1))
		return ERR_INVALID_PARAMETER;

	// 获取屏幕像素流
	CAutoRef<MemoryBitmap> memoryBitmap = NULL;
	int ret = m_colorAssist->GetBitmapStream(bitmapHandle, x, y, width, height, memoryBitmap);
	if (!IS_SUCCESS(ret))
		return ret;

	uint32_t *pixels = (uint32_t *)memoryBitmap->GetBits();
	int minX = width, maxX = 0;
	int minY = height, maxY = 0;
	// 去掉文字周围的空白区域，找出包含文字的区域
	for (int y1 = 0; y1 < height; y1++)
	{
		for (int x1 = 0; x1 < width; x1++)
		{
			if (colorParser.IsSimilarColor(*(RGBQUAD *)&pixels[y1 * width + x1]))
			{
				if (x1 < minX) minX = x1;
				if (x1 >= maxX) maxX = x1 + 1;
				if (y1 < minY) minY = y1;
				if (y1 >= maxY) maxY = y1 + 1;
			}
		}
	}

	if (maxX == 0 || maxY == 0)
		return ERR_AREA_INVALID;

	int binaryAreaWidth = maxX - minX, binaryAreaHeight = maxY - minY;
	uint8_t *binaryArea = new uint8_t[2 + 2 + binaryAreaWidth * binaryAreaHeight / 8 + 1];
	memset(binaryArea, 0, 2 + 2 + binaryAreaWidth * binaryAreaHeight / 8 + 1);

	uint8_t *outPtr = binaryArea;
	// 如果值小于247就只使用一个字节来记录
	if (binaryAreaWidth < 247)
		*outPtr++ = (uint8_t)binaryAreaWidth;
	else
	{
		// 当值大于247时使用两个字节来记录该值，记录范围为0至(255-247)*256=2047
		*outPtr++ = 247 + (uint8_t)(binaryAreaWidth / 256);
		*outPtr++ = (uint8_t)(binaryAreaWidth % 256);
	}
	if (binaryAreaHeight < 247)
		*outPtr++ = (uint8_t)binaryAreaHeight;
	else
	{
		*outPtr++ = 247 + (uint8_t)(binaryAreaHeight / 256);
		*outPtr++ = (uint8_t)(binaryAreaHeight % 256);
	}

	int bitCount = 0;
	for (int y2 = minY; y2 < maxY; y2++)
	{
		for (int x2 = minX; x2 < maxX; x2++)
		{
			if (colorParser.IsSimilarColor(*(RGBQUAD *)&pixels[y2 * width + x2]))
				*outPtr |= 1 << bitCount;
			if (++bitCount > 7)
			{
				outPtr++;
				bitCount = 0;
			}
		}
	}
	if (bitCount)
		outPtr++;

	CBase64 base64 = CBase64::Encode(binaryArea, outPtr - binaryArea);
	delete [] binaryArea;
	strResult = (const char *)base64.GetBuffer();
	return ERR_NONE;
}

static bool OcrFoundCharCmp(const OCR_FOUND_CHAR &a, const OCR_FOUND_CHAR &b)
{
	// 判断两个字是否在同一行，是就根据X坐标确定前后关系
	if (a.FoundY == b.FoundY)
		return a.FoundX < b.FoundX;
	// 判断字a是否在字b所在行中
	if (a.FoundY > b.FoundY && a.FoundY < b.FoundY + b.FontHeight)
		return a.FoundX < b.FoundX;
	// 判断字b是否在字a所在行中
	if (b.FoundY > a.FoundY && b.FoundY < a.FoundY + a.FontHeight)
		return a.FoundX < b.FoundX;
	// 如果两个字不是在同一行，就根据Y坐标确定前后关系
	return a.FoundY < b.FoundY;
}

int SimpleOcr::Ocr(handle_t bitmapHandle, int x, int y, int width, int height, const char *dictName, const char *colorString, float similar, std::string &strResult)
{
	if (dictName == NULL || colorString == NULL)
		return ERR_INVALID_PARAMETER;

	OCR_DICTS::iterator iter = m_ocrDicts.find(dictName);
	if (iter == m_ocrDicts.end())
		return ERR_NAME_NOT_EXIST;		// 找不到字典
	PFONT_BITS font = iter->second;

	// 获取屏幕像素流
	CAutoRef<MemoryBitmap> memoryBitmap = NULL;
	int ret = m_colorAssist->GetBitmapStream(bitmapHandle, x, y, width, height, memoryBitmap);
	if (!IS_SUCCESS(ret))
		return ret;

	uint32_t *pixels = (uint32_t *)memoryBitmap->GetBits();
	FontSearchEx fontSearch;
	if (!fontSearch.InitImage(pixels, width, height, colorString))
		return ERR_INVOKE_FAILED;

	std::vector<OCR_FOUND_CHAR> foundChars;

	do
	{
		int startX = 0, startY = 0;
		while (fontSearch.FullSearchFont(font, similar, startX, startY, false))
		{
			OCR_FOUND_CHAR foundChar = {NULL, startX, startY, font->Height};
			if (font->CharCodeIsOffset)
				foundChar.CharText = (const char *)((uint8_t *)font + font->CharCode);
			else
				foundChar.CharText = (const char *)&(font->CharCode);
			foundChars.push_back(foundChar);
			// 清除已识别字体所在区域，这样可加快后面的识别速度
			fontSearch.ClearFontArea(startX, startY, font->Width, font->Height);
			startX += font->Width;
			if (startX + font->Width > width)
			{
				startX = 0;
				startY += font->Height;
			}
		}
	} while ((font = GetNextFontBits(font)));

	// 把所有找到的字体根据坐标进行排序，得到一个完整的字符串
	std::sort(foundChars.begin(), foundChars.end(), OcrFoundCharCmp);
	strResult = "";
	for (size_t i = 0; i < foundChars.size(); i++)
	{
		if (i > 0 && foundChars[i - 1].FoundY + foundChars[i - 1].FontHeight <= foundChars[i].FoundY)
			strResult += "\r\n";
		strResult += foundChars[i].CharText;
	}

	return ERR_NONE;
}

int SimpleOcr::OcrEx(handle_t bitmapHandle, int x, int y, int width, int height, const char *dictName, const char *colorString, float similar, bool checkSpace, std::string &strResult)
{
	if (dictName == NULL || colorString == NULL)
		return ERR_INVALID_PARAMETER;

	OCR_DICTS::iterator iter = m_ocrDicts.find(dictName);
	if (iter == m_ocrDicts.end())
		return ERR_NAME_NOT_EXIST;		// 找不到字典
	PFONT_BITS font = iter->second;

	// 获取屏幕像素流
	CAutoRef<MemoryBitmap> memoryBitmap = NULL;
	int ret = m_colorAssist->GetBitmapStream(bitmapHandle, x, y, width, height, memoryBitmap);
	if (!IS_SUCCESS(ret))
		return ret;

	uint32_t *pixels = (uint32_t *)memoryBitmap->GetBits();
	FontSearchEx fontSearch;
	if (!fontSearch.InitImage(pixels, width, height, colorString))
		return ERR_INVOKE_FAILED;

	std::vector<OCR_FOUND_CHAR> foundChars;

	do
	{
		int startX = 0, startY = 0;
		while (fontSearch.FullSearchFont(font, similar, startX, startY, checkSpace))
		{
			OCR_FOUND_CHAR foundChar = {NULL, startX, startY, font->Height};
			if (font->CharCodeIsOffset)
				foundChar.CharText = (const char *)((uint8_t *)font + font->CharCode);
			else
				foundChar.CharText = (const char *)&(font->CharCode);
			foundChars.push_back(foundChar);
			// 清除已识别字体所在区域，这样可加快后面的识别速度
			fontSearch.ClearFontArea(startX, startY, font->Width, font->Height);
			startX += font->Width;
			if (startX + font->Width > width)
			{
				startX = 0;
				startY += font->Height;
			}
		}
	} while ((font = GetNextFontBits(font)));

	// 把所有找到的字体根据坐标进行排序，得到一个完整的字符串
	std::sort(foundChars.begin(), foundChars.end(), OcrFoundCharCmp);
	strResult = "";
	for (size_t i = 0; i < foundChars.size(); i++)
	{
		if (!strResult.empty())
			strResult += "\r\n";
		strResult += format("%d|%d|%s", x + foundChars[i].FoundX, y + foundChars[i].FoundY, foundChars[i].CharText);
	}

	return ERR_NONE;
}
