#include <fcntl.h>
#include <unistd.h>
#include <getopt.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <errno.h>
#include "common.h"
#include "screenreaderbyfb.h"

ScreenReaderByFb::ScreenReaderByFb()
{

}

ScreenReaderByFb::~ScreenReaderByFb()
{

}

int ScreenReaderByFb::GetScreenInfo()
{
	const char *path = "/dev/graphics/fb0";
	int fd = open(path, O_RDONLY);
	if (fd < 0)
	{
		LOGI("Cannot open %s\n", path);
		return ERR_INVOKE_FAILED;
	}

	fb_var_screeninfo vinfo;
	if (ioctl(fd, FBIOGET_VSCREENINFO, &vinfo) < 0)
	{
		close(fd);
		LOGI("Cannot get FBIOGET_VSCREENINFO of %s\n", path);
		return ERR_INVOKE_FAILED;
	}
	close(fd);

	m_frameWidth = vinfo.xres;
	m_frameHeight = vinfo.yres;
	m_frameBpp = vinfo.bits_per_pixel;
	return ERR_NONE;
}

int ScreenReaderByFb::CaptureFrame()
{
	const char *path = "/dev/graphics/fb0";
	int fd = open(path, O_RDONLY);
	if (fd < 0)
	{
		LOGI("Cannot open fb device %s, err=%d\n", path, errno);
		return ERR_INVOKE_FAILED;
	}

	fb_var_screeninfo vinfo;
	if (ioctl(fd, FBIOGET_VSCREENINFO, &vinfo) < 0)
	{
		close(fd);
		LOGI("Cannot get FBIOGET_VSCREENINFO of %s\n", path);
		return ERR_INVOKE_FAILED;
	}

	fb_fix_screeninfo finfo;
	if (ioctl(fd, FBIOGET_FSCREENINFO, &finfo) != 0)
	{
		close(fd);
		LOGI("Cannot get FBIOGET_FSCREENINFO of %s\n", path);
		return ERR_INVOKE_FAILED;
	}

	LOGI("line_lenght=%d, xres=%d, yres=%d, xresv=%d, yresv=%d, xoffs=%d, yoffs=%d, bpp=%d, rotate=%d\n",
	         (int)finfo.line_length,(int)vinfo.xres, (int)vinfo.yres,
	         (int)vinfo.xres_virtual, (int)vinfo.yres_virtual,
	         (int)vinfo.xoffset, (int)vinfo.yoffset,
	         (int)vinfo.bits_per_pixel, (int)vinfo.rotate);

	size_t size = vinfo.yres_virtual;
	if (size < vinfo.yres * 2)
	{
		LOGI("Using Droid workaround\n");
		size = vinfo.yres * 2;
	}

	if (vinfo.bits_per_pixel == 24)
	{
		vinfo.bits_per_pixel = 32;
		LOGI("24-bit XRGB display detected\n");
	}

	size_t fbSize = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;
//	size_t fbSize = (finfo.line_length * size + (PAGE_SIZE-1)) & ~(PAGE_SIZE - 1);
	void *fbmmap = mmap(nullptr, fbSize, PROT_READ, MAP_SHARED, fd, 0);
	close(fd);

	if (fbmmap == MAP_FAILED)
	{
		LOGI("Cannot map fb memory, err=%d\n", errno);
		return ERR_INVOKE_FAILED;
	}

	m_frameWidth = vinfo.xres;
	m_frameHeight = vinfo.yres;
	m_frameBpp = vinfo.bits_per_pixel;
	m_frameData = fbmmap;
	m_frameSize = fbSize;
	return ERR_NONE;
}

void ScreenReaderByFb::ReleaseFrameBuffer()
{
	if (m_frameData)
		munmap((void *)m_frameData, m_frameSize);

	m_frameWidth = 0;
	m_frameHeight = 0;
	m_frameBpp = 0;
	m_frameData = nullptr;
	m_frameSize = 0;
}
