#ifndef _VIRTDISPLAYREADER_H
#define _VIRTDISPLAYREADER_H

#include "screenreader.h"

class VirtDisplayReader : public ScreenReader
{
public:
	virtual int CaptureScreen(int x, int y, int width, int height, void *outBuf);

	VirtDisplayReader();
	virtual ~VirtDisplayReader();

protected:
	virtual int CaptureFrame();
	virtual void ReleaseFrameBuffer();
	virtual int GetRotation();

private:
	int InitVirtDisplay(int width, int height);
	void UninitVirtDisplay();
	int GetScreenInfo(int &width, int &height);

	static void *FrameRefreshThread(void *arg);
};

#endif
