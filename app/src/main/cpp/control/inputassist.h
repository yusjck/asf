#ifndef _INPUTASSIST_H
#define _INPUTASSIST_H

#include "deviceinfo.h"

#define BUS_VIRTUAL 0x06

class InputAssist
{
public:
	int KeyDown(int iVirtKey);
	int KeyUp(int iVirtKey);
	int KeyPress(int iVirtKey, uint32_t duration);
	int MouseMove(int iX, int iY, int iFlags);
	int MouseDown(int iVirtKey);
	int MouseUp(int iVirtKey);
	int MouseClick(int iVirtKey, uint32_t duration);
	int TouchMove(int iPointId, int iX, int iY, int iFlags);
	int TouchDown(int iPointId, int iX, int iY);
	int TouchUp(int iPointId);
	int TouchTap(int iPointId, int iX, int iY);
	int TouchSwipe(int iPointId, int iX1, int iY1, int iX2, int iY2, uint32_t duration);
	int SendString(uint32_t iSendMode, const char *pContent);
	InputAssist();
	~InputAssist();

public:
	std::shared_ptr<DeviceInfo> m_deviceInfo;

private:
	void transform_touch_xy(int *x, int *y);
};

#endif
