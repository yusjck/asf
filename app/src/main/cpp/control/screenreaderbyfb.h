#ifndef _FBREADER_H
#define _FBREADER_H

#include "screenreader.h"

class ScreenReaderByFb : public ScreenReader
{
public:
	ScreenReaderByFb();
	virtual ~ScreenReaderByFb();

protected:
	virtual int CaptureFrame();
	virtual void ReleaseFrameBuffer();

private:
	int GetScreenInfo();
};

#endif
