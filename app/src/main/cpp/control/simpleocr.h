//
// Created by yusjc on 2020/5/4.
//

#ifndef ASF_SIMPLEOCR_H
#define ASF_SIMPLEOCR_H

#include <map>
#include <string>

class ColorAssist;

class SimpleOcr
{
public:
	int FindString(handle_t bitmapHandle, int x, int y, int width, int height, const char *text, const char *dictName, int maxResult, std::string &strResult);
	int FindStringEx(handle_t bitmapHandle, int x, int y, int width, int height, const char *text, const char *dictName, const char *colorString, float similar, int maxResult, std::string &strResult);
	int SetOcrDict(const char *dictName, int dictType, const char *dict);
	int OcrExtract(handle_t bitmapHandle, int x, int y, int width, int height, const char *colorString, std::string &strResult);
	int Ocr(handle_t bitmapHandle, int x, int y, int width, int height, const char *dictName, const char *colorString, float similar, std::string &strResult);
	int OcrEx(handle_t bitmapHandle, int x, int y, int width, int height, const char *dictName, const char *colorString, float similar, bool checkSpace, std::string &strResult);

	SimpleOcr(ColorAssist *colorAssist);
	~SimpleOcr();

private:

	typedef struct _FONT_BITS FONT_BITS, *PFONT_BITS;
	typedef std::map<std::string, PFONT_BITS> OCR_DICTS;

	ColorAssist *m_colorAssist;
	OCR_DICTS m_ocrDicts;

	PFONT_BITS GetFontBits(const char *text, const char *dictName);
	PFONT_BITS LoadFontBits(const char *fontData);
	PFONT_BITS GetNextFontBits(PFONT_BITS fontBits);
	void ReleaseFontBits(PFONT_BITS fontBits);
	bool SaveFontBits(PFONT_BITS fontBits, std::string &strResult);
};

#endif //ASF_SIMPLEOCR_H
