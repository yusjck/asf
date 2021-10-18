#include <unistd.h>
#include "common.h"
#include "deviceinfo.h"
#include "androidcaller.h"
#include "virtdisplayreader.h"
#include "screenreaderbyfb.h"
#include "controller.h"

int DeviceInfo::m_displayWidth = 0;
int DeviceInfo::m_displayHeight = 0;
int DeviceInfo::m_rotation = 0;

DeviceInfo::DeviceInfo(Controller *controller)
{
	m_controller = controller;
}

DeviceInfo::~DeviceInfo()
= default;

int DeviceInfo::SetCaptureMode(int mode)
{
	switch (mode)
	{
		case 0:
			m_screenReader = std::make_unique<ScreenReader>();
			break;
		case 1:
			m_screenReader = std::make_unique<VirtDisplayReader>();
			break;
		case 2:
			m_screenReader = std::make_unique<ScreenReaderByFb>();
			break;
		default:
			return ERR_INVALID_PARAMETER;
	}
	return ERR_NONE;
}

int DeviceInfo::ScreenShot(int x, int y, int width, int height, void *outBuf)
{
	// ROOT权限下可以直接截屏
	if (getuid() == 0)
	{
		if (m_screenReader == nullptr)
			SetCaptureMode(0);
		m_screenReader->SetRotation(m_rotation);
		return m_screenReader->CaptureScreen(x, y, width, height, outBuf);
	}

	CallerContext ctx(ACCMD_SCREENSHOT);
	ctx.WriteInt(x);
	ctx.WriteInt(y);
	ctx.WriteInt(width);
	ctx.WriteInt(height);

	int ret = m_controller->m_androidCaller.Call(ctx);
	if (!IS_SUCCESS(ret))
		return ret;

	uint32_t len;
	const void *buf = ctx.ReadBin(len);
	if (width * height * 4 != len)  // 检查截屏后返回的像素缓冲区长度是否正确
		return ERR_INVOKE_FAILED;

	memcpy(outBuf, buf, len);
	return ERR_NONE;
}

int DeviceInfo::GetDisplayInfo(int &width, int &height, int &rotation)
{
	if (m_displayWidth == 0)
	{
		int ret = m_controller->m_androidCaller.GetDisplayInfo(m_displayWidth, m_displayHeight, m_rotation);
		if (!IS_SUCCESS(ret))
			return ret;
	}

	width = m_displayWidth;
	height = m_displayHeight;
	rotation = m_rotation;
	return ERR_NONE;
}

int DeviceInfo::UpdateDisplayInfo(int width, int height, int rotation)
{
	switch (rotation)
	{
		case 0:
		case 90:
		case 180:
		case 270:
			break;
		default:
			return ERR_INVALID_PARAMETER;
	}

	m_displayWidth = width;
	m_displayHeight = height;
	m_rotation = rotation;
	return ERR_NONE;
}
