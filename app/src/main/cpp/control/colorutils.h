#ifndef _COLORUTILS_H
#define _COLORUTILS_H

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <string>
#include <vector>
#include "strutil.h"

typedef union tagRGBQUAD {
	struct {
		uint8_t rgbBlue;
		uint8_t rgbGreen;
		uint8_t rgbRed;
		uint8_t rgbReserved;
	};
	uint32_t rgbQuad;
} RGBQUAD;

typedef union _HSVQUAD {
	struct {
		short H;
		char S;
		char V;
	};
	uint32_t hsvQuad;
} HSVQUAD, *PHSVQUAD;

#define NOHUE   -1

/* Convert one RGB pixel into HSV space. Borrowed from Xv */
static void rgb2hsv(int r, int g, int b, double *hr, double *sr, double *vr)
{
	double rd, gd, bd, h, s, v, max, min, del, rc, gc, bc;

	/* convert RGB to HSV */
	rd = r / 255.0;            /* rd,gd,bd range 0-1 instead of 0-255 */
	gd = g / 255.0;
	bd = b / 255.0;

	/* compute maximum of rd,gd,bd */
	if (rd >= gd)
	{ if (rd >= bd) max = rd; else max = bd; }
	else
	{ if (gd >= bd) max = gd; else max = bd; }

	/* compute minimum of rd,gd,bd */
	if (rd <= gd)
	{ if (rd <= bd) min = rd; else min = bd; }
	else
	{ if (gd <= bd) min = gd; else min = bd; }

	del = max - min;
	v = max;
	if (max != 0.0) s = (del) / max;
	else s = 0.0;

	h = NOHUE;
	if (s != 0.0)
	{
		rc = (max - rd) / del;
		gc = (max - gd) / del;
		bc = (max - bd) / del;

		if (rd == max) h = bc - gc;
		else if (gd == max) h = 2 + rc - bc;
		else if (bd == max) h = 4 + gc - rc;

		h = h * 60;
		if (h < 0) h += 360;
	}

	*hr = h;
	*sr = s;
	*vr = v;
}

class ColorParser
{
private:
	typedef union _COLOR_INFO {
		struct {
			RGBQUAD rgbColorCmp;
			RGBQUAD rgbColorRange;
		};
		struct {
			HSVQUAD hsvColorCmp;
			HSVQUAD hsvColorRange;
		};
	} COLOR_INFO, *PCOLOR_INFO;

public:
	bool ParseColorString(const char *colorString, float similar)
	{
		if (colorString == NULL)
			return false;

		// 如果颜色描述是用b@开头，表示后面颜色用于匹配背景色，需要反转匹配结果
		if (colorString[0] == 'b' && colorString[1] == '@')
		{
			m_reverseColor = true;
			colorString += 2;
		}
		else
			m_reverseColor = false;

		// 如果颜色使用100.20.20这样的格式表示这是一个HSV颜色值，否则作为RGB颜色处理
		if (strchr(colorString, '.'))
			m_colorType = 1;
		else
			m_colorType = 0;

		int iSimilar;
		// 相似度大于1的时候，把它当做单色上下差的值
		if (similar > 1)
			iSimilar = (int)similar;
		else
			iSimilar = (int)(255 * (1 - similar));

		std::vector<COLOR_INFO> colorInfos;
		std::vector<std::string> colorStrs = split(colorString, "|");
		for (int i = 0; i < colorStrs.size(); i++)
		{
			COLOR_INFO colorInfo = {0};
			if (m_colorType == 1)
			{
				int h = 0, s = 0, v = 0, h1 = 0, s1 = 0, v1 = 0;
				sscanf(colorStrs[i].c_str(), "%d.%d.%d-%d.%d.%d", &h, &s, &v, &h1, &s1, &v1);

				colorInfo.hsvColorCmp.H = h;
				colorInfo.hsvColorCmp.S = s;
				colorInfo.hsvColorCmp.V = v;

				colorInfo.hsvColorRange.H = h1;
				colorInfo.hsvColorRange.S = s1;
				colorInfo.hsvColorRange.V = v1;

				if (colorInfo.hsvColorRange.hsvQuad == 0)
				{
					colorInfo.hsvColorRange.H = 360 * similar;
					colorInfo.hsvColorRange.S = 100 * similar;
					colorInfo.hsvColorRange.V = 100 * similar;
				}
			}
			else
			{
				int r = 0, g = 0, b = 0, rc = 0, gc = 0, bc = 0;
				// 这里读取rgb值存在大端小端差异，需要特别注意
				sscanf(colorStrs[i].c_str(), "%02x%02x%02x-%02x%02x%02x", &b, &g, &r, &bc, &gc, &rc);

				colorInfo.rgbColorCmp.rgbBlue = b;
				colorInfo.rgbColorCmp.rgbGreen = g;
				colorInfo.rgbColorCmp.rgbRed = r;

				colorInfo.rgbColorRange.rgbBlue = bc;
				colorInfo.rgbColorRange.rgbGreen = gc;
				colorInfo.rgbColorRange.rgbRed = rc;

				if (colorInfo.rgbColorRange.rgbQuad == 0)
				{
					colorInfo.rgbColorRange.rgbBlue = iSimilar;
					colorInfo.rgbColorRange.rgbGreen = iSimilar;
					colorInfo.rgbColorRange.rgbRed = iSimilar;
				}
			}
			colorInfos.push_back(colorInfo);
		}

		if (colorInfos.size() == 0)
			return false;

		if (m_colorInfos)
		{
			delete[] m_colorInfos;
			m_colorInfos = NULL;
			m_colorInfoNum = 0;
		}

		// 分配内存用于存放颜色数组，这样访问速度会更快一些
		m_colorInfoNum = colorInfos.size();
		m_colorInfos = new COLOR_INFO[m_colorInfoNum];
		for (int i = 0; i < m_colorInfoNum; i++)
		{
			m_colorInfos[i] = colorInfos[i];
		}
		return true;
	}

	bool IsSimilarColor(RGBQUAD rgbColor)
	{
		if (m_colorType == 1)
		{
			// 使用HSV空间对比两种颜色
			double fh, fs, fv;
			// 先将要比较的颜色转换成HSV格式
			rgb2hsv(rgbColor.rgbRed, rgbColor.rgbGreen, rgbColor.rgbBlue, &fh, &fs, &fv);
			// H取0-360范围内的整数，S和V取0-100范围内的整数
			short h = fh + 0.5;
			char s = fs * 100 + 0.5;
			char v = fv * 100 + 0.5;
			for (uint32_t i = 0; i < m_colorInfoNum; i++)
			{
				if (abs(h - m_colorInfos[i].hsvColorCmp.H) <= m_colorInfos[i].hsvColorRange.H &&
				    abs(s - m_colorInfos[i].hsvColorCmp.S) <= m_colorInfos[i].hsvColorRange.S &&
				    abs(v - m_colorInfos[i].hsvColorCmp.V) <= m_colorInfos[i].hsvColorRange.V)
					return m_reverseColor ? false : true;
			}
			return m_reverseColor ? true : false;
		}
		else
		{
			// 使用RGB空间对比两种颜色
			for (uint32_t i = 0; i < m_colorInfoNum; i++)
			{
				if (abs(rgbColor.rgbBlue - m_colorInfos[i].rgbColorCmp.rgbBlue) <= m_colorInfos[i].rgbColorRange.rgbBlue &&
				    abs(rgbColor.rgbGreen - m_colorInfos[i].rgbColorCmp.rgbGreen) <= m_colorInfos[i].rgbColorRange.rgbGreen &&
				    abs(rgbColor.rgbRed - m_colorInfos[i].rgbColorCmp.rgbRed) <= m_colorInfos[i].rgbColorRange.rgbRed)
					return m_reverseColor ? false : true;
			}
			return m_reverseColor ? true : false;
		}
	}

public:
	ColorParser()
	{
		m_colorType = 0;
		m_reverseColor = false;
		m_colorInfos = NULL;
		m_colorInfoNum = 0;
	}

	~ColorParser()
	{
		if (m_colorInfos)
			delete[] m_colorInfos;
	}

private:
	ColorParser(const ColorParser &C);

	uint32_t m_colorType;        // 0为RGB，1为HSV
	bool m_reverseColor;        // 是否为背景色
	PCOLOR_INFO m_colorInfos;
	uint32_t m_colorInfoNum;
};

class ColorParserEx
{
private:
	typedef struct _COLOR_INFO_EX {
		union {
			struct {
				RGBQUAD rgbColorCmp;
				RGBQUAD rgbColorRange;
			};
			struct {
				HSVQUAD hsvColorCmp;
				HSVQUAD hsvColorRange;
			};
		};
		int OffsetX;
		int OffsetY;
		int PixelTotal;
	} COLOR_INFO_EX, *PCOLOR_INFO_EX;

	static int ColorArrayCmp(const void *a, const void *b)
	{
		return ((PCOLOR_INFO_EX)a)->PixelTotal - ((PCOLOR_INFO_EX)b)->PixelTotal;
	}

	void SortColorArray(RGBQUAD *rgbColors, uint32_t color)
	{
		for (int x = 0; x < m_colorInfoNum; x++)
		{
			m_colorInfos[x].PixelTotal = 0;
		}
		for (int i = 0; i < color; i++)
		{
			for (int x = 0; x < m_colorInfoNum; x++)
			{
				if (IsSimilarColor(rgbColors[i], &m_colorInfos[x]))
				{
					m_colorInfos[x].PixelTotal++;
				}
			}
		}
		qsort(m_colorInfos, m_colorInfoNum, sizeof(COLOR_INFO_EX), ColorArrayCmp);
	}

	__inline bool IsSimilarColor(RGBQUAD rgbColor, PCOLOR_INFO_EX colorInfo)
	{
		if (m_colorType == 1)
		{
			// 使用HSV空间对比两种颜色
			double fh, fs, fv;
			// 先将要比较的颜色转换成HSV格式
			rgb2hsv(rgbColor.rgbRed, rgbColor.rgbGreen, rgbColor.rgbBlue, &fh, &fs, &fv);
			// H取0-360范围内的整数，S和V取0-100范围内的整数
			short h = fh + 0.5;
			char s = fs * 100 + 0.5;
			char v = fv * 100 + 0.5;
			if (abs(h - colorInfo->hsvColorCmp.H) <= colorInfo->hsvColorRange.H &&
			    abs(s - colorInfo->hsvColorCmp.S) <= colorInfo->hsvColorRange.S &&
			    abs(v - colorInfo->hsvColorCmp.V) <= colorInfo->hsvColorRange.V)
				return m_reverseColor ? false : true;
			return m_reverseColor ? true : false;
		}
		else
		{
			if (abs(rgbColor.rgbBlue - colorInfo->rgbColorCmp.rgbBlue) <= colorInfo->rgbColorRange.rgbBlue &&
			    abs(rgbColor.rgbGreen - colorInfo->rgbColorCmp.rgbGreen) <= colorInfo->rgbColorRange.rgbGreen &&
			    abs(rgbColor.rgbRed - colorInfo->rgbColorCmp.rgbRed) <= colorInfo->rgbColorRange.rgbRed)
				return m_reverseColor ? false : true;
			return m_reverseColor ? true : false;
		}
	}

public:
	bool ParseColorString(const char *colorString, float similar)
	{
		if (colorString == NULL)
			return false;

		// 如果颜色描述是用b@开头，表示后面颜色用于匹配背景色，需要反转匹配结果
		if (colorString[0] == 'b' && colorString[1] == '@')
		{
			m_reverseColor = true;
			colorString += 2;
		}
		else
			m_reverseColor = false;

		// 如果颜色使用100.20.20这样的格式表示这是一个HSV颜色值，否则作为RGB颜色处理
		if (strchr(colorString, '.'))
			m_colorType = 1;
		else
			m_colorType = 0;

		int iSimilar;
		// 相似度大于1的时候，把它当做单色上下差的值
		if (similar > 1)
			iSimilar = (int)similar;
		else
			iSimilar = (int)(255 * (1 - similar));

		m_maxLeftOffset = 0;
		m_maxTopOffset = 0;
		m_maxRightOffset = 0;
		m_maxBottomOffset = 0;

		std::vector<COLOR_INFO_EX> colorInfos;
		std::vector<std::string> colorStrs = split(colorString, ",");
		for (int i = 0; i < colorStrs.size(); i++)
		{
			std::vector<std::string> colorInfoStrs = split(colorStrs[i], "|");
			if (colorInfoStrs.size() != 3)
				continue;

			COLOR_INFO_EX colorInfo = {0};
			colorInfo.OffsetX = atoi(colorInfoStrs[0].c_str());
			colorInfo.OffsetY = atoi(colorInfoStrs[1].c_str());

			if (m_colorType == 1)
			{
				int h = 0, s = 0, v = 0, h1 = 0, s1 = 0, v1 = 0;
				sscanf(colorInfoStrs[2].c_str(), "%d.%d.%d-%d.%d.%d", &h, &s, &v, &h1, &s1, &v1);

				colorInfo.hsvColorCmp.H = h;
				colorInfo.hsvColorCmp.S = s;
				colorInfo.hsvColorCmp.V = v;

				colorInfo.hsvColorRange.H = h1;
				colorInfo.hsvColorRange.S = s1;
				colorInfo.hsvColorRange.V = v1;

				if (colorInfo.hsvColorRange.hsvQuad == 0)
				{
					colorInfo.hsvColorRange.H = 360 * similar;
					colorInfo.hsvColorRange.S = 100 * similar;
					colorInfo.hsvColorRange.V = 100 * similar;
				}
			}
			else
			{
				int r = 0, g = 0, b = 0, rc = 0, gc = 0, bc = 0;
				// 这里读取rgb值存在大端小端差异，需要特别注意
				sscanf(colorInfoStrs[2].c_str(), "%02x%02x%02x-%02x%02x%02x", &b, &g, &r, &bc, &gc, &rc);

				colorInfo.rgbColorCmp.rgbBlue = b;
				colorInfo.rgbColorCmp.rgbGreen = g;
				colorInfo.rgbColorCmp.rgbRed = r;

				colorInfo.rgbColorRange.rgbBlue = bc;
				colorInfo.rgbColorRange.rgbGreen = gc;
				colorInfo.rgbColorRange.rgbRed = rc;

				if (colorInfo.rgbColorRange.rgbQuad == 0)
				{
					colorInfo.rgbColorRange.rgbBlue = iSimilar;
					colorInfo.rgbColorRange.rgbGreen = iSimilar;
					colorInfo.rgbColorRange.rgbRed = iSimilar;
				}
			}
			colorInfos.push_back(colorInfo);

			// 计算出这个多点字串组成的这个矩形以目标点向4个方向延伸需各占多少像素
			if (colorInfo.OffsetX < 0)
			{
				if (-colorInfo.OffsetX > m_maxLeftOffset)
					m_maxLeftOffset = -colorInfo.OffsetX;
			}
			else
			{
				if (colorInfo.OffsetX > m_maxRightOffset)
					m_maxRightOffset = colorInfo.OffsetX;
			}
			if (colorInfo.OffsetY < 0)
			{
				if (-colorInfo.OffsetY > m_maxTopOffset)
					m_maxTopOffset = -colorInfo.OffsetY;
			}
			else
			{
				if (colorInfo.OffsetY > m_maxBottomOffset)
					m_maxBottomOffset = colorInfo.OffsetY;
			}
		}

		if (colorInfos.size() == 0)
			return false;

		if (m_colorInfos)
		{
			delete[] m_colorInfos;
			m_colorInfos = NULL;
			m_colorInfoNum = 0;
		}

		m_colorInfoNum = colorInfos.size();
		m_colorInfos = new COLOR_INFO_EX[m_colorInfoNum];
		for (int i = 0; i < m_colorInfoNum; i++)
		{
			m_colorInfos[i] = colorInfos[i];
		}
		return true;
	}

	bool IsSimilarColors(RGBQUAD *rgbColors, int x, int y, int width, int height)
	{
		if (x - m_maxLeftOffset < 0 || x + m_maxRightOffset >= width)
			return false;
		if (y - m_maxTopOffset < 0 || y + m_maxBottomOffset >= height)
			return false;

		for (uint32_t i = 0; i < m_colorInfoNum; i++)
		{
			RGBQUAD rgbColor = rgbColors[(y + m_colorInfos[i].OffsetY) * width + x + m_colorInfos[i].OffsetX];
			if (!IsSimilarColor(rgbColor, &m_colorInfos[i]))
				return false;
		}
		return true;
	}

public:
	ColorParserEx()
	{
		m_colorType = 0;
		m_reverseColor = false;
		m_colorInfos = NULL;
		m_colorInfoNum = 0;
		m_maxLeftOffset = 0;
		m_maxTopOffset = 0;
		m_maxRightOffset = 0;
		m_maxBottomOffset = 0;
	}

	~ColorParserEx()
	{
		if (m_colorInfos)
			delete[] m_colorInfos;
	}

private:
	ColorParserEx(const ColorParserEx &C);

	uint32_t m_colorType;        // 0为RGB，1为HSV
	bool m_reverseColor;        // 是否为背景色
	PCOLOR_INFO_EX m_colorInfos;
	uint32_t m_colorInfoNum;
	int m_maxLeftOffset;                // 4个坐标使用优化搜索速度
	int m_maxTopOffset;
	int m_maxRightOffset;
	int m_maxBottomOffset;
};

#endif
