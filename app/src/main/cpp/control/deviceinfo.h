//
// Created by Jack on 2019/6/11.
//

#ifndef ASF_DEVICEINFO_H
#define ASF_DEVICEINFO_H


class Controller;
class ScreenReader;

class DeviceInfo
{
public:
	// 返回屏幕旋转后的分辨率
	int GetDisplayInfo(int &width, int &height, int &rotation);
	int SetCaptureMode(int mode);
	int ScreenShot(int x, int y, int width, int height, void *outBuf);
	static int UpdateDisplayInfo(int width, int height, int rotation);

	DeviceInfo(Controller *controller);
	~DeviceInfo();

private:
	Controller *m_controller;
	std::unique_ptr<ScreenReader> m_screenReader;

	static int m_displayWidth;
	static int m_displayHeight;
	static int m_rotation;
};


#endif //ASF_DEVICEINFO_H
