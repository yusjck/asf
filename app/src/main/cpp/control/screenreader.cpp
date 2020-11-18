#include "common.h"
#include "screenreader.h"

ScreenReader::ScreenReader()
{
	m_rotation = 0;
	m_frameBuf = nullptr;
	m_frameWidth = 0;
	m_frameHeight = 0;
	m_frameBpp = 0;
	m_frameData = nullptr;
	m_frameSize = 0;
}

ScreenReader::~ScreenReader()
{
	ReleaseFrameBuffer();
}

int ScreenReader::CaptureScreen(int x, int y, int width, int height, void *outBuf)
{
	int res = CaptureFrame();
	if (!IS_SUCCESS(res))
		return res;

	CopyFrameBuffer(x, y, width, height, outBuf);
	ReleaseFrameBuffer();
	return ERR_NONE;
}

int ScreenReader::CaptureFrame()
{
	FILE *pd = popen("/system/bin/screencap", "r");
	if (pd == nullptr)
	{
		LOGI("screencap: open failed\n");
		return ERR_INVOKE_FAILED;
	}

	int w, h, f;
	if (fread((void *)&w, 1, sizeof(int), pd) != sizeof(int) ||
	    fread((void *)&h, 1, sizeof(int), pd) != sizeof(int) ||
	    fread((void *)&f, 1, sizeof(int), pd) != sizeof(int))
	{
		pclose(pd);
		LOGI("screencap: read failed\n");
		return ERR_INVOKE_FAILED;
	}

	LOGI("screencap: w=%d,h=%d,f=%d\n", w, h, f);
	if (f != 1)
	{
		pclose(pd);
		LOGI("screencap: invalid format\n");
		return ERR_INVOKE_FAILED;
	}

	ReleaseFrameBuffer();

	size_t fbSize = w * h * 4;
	m_frameBuf = malloc(fbSize);
	if (m_frameBuf == nullptr)
	{
		pclose(pd);
		LOGI("screencap: no memory\n");
		return ERR_ALLOC_MEMORY_FAILED;
	}

	fread(m_frameBuf, 1, fbSize, pd);
	pclose(pd);

	m_frameWidth = w;
	m_frameHeight = h;
	m_frameBpp = 32;
	m_frameData = m_frameBuf;
	m_frameSize = fbSize;
	return ERR_NONE;
}

void ScreenReader::ReleaseFrameBuffer()
{
	if (m_frameBuf != nullptr)
	{
		free(m_frameBuf);
		m_frameBuf = nullptr;
	}

	m_frameWidth = 0;
	m_frameHeight = 0;
	m_frameBpp = 0;
	m_frameData = nullptr;
	m_frameSize = 0;
}

void ScreenReader::SetRotation(int rotation)
{
	m_rotation = rotation;
}

int ScreenReader::GetRotation()
{
	return m_rotation;
}

void ScreenReader::CopyFrameBuffer(int dstX, int dstY, int dstWidth, int dstHeight, void *dstBuf)
{
	int rotation = GetRotation();
	if (rotation == 0)
	{
		for (int y = 0; y < dstHeight; y++)
		{
			int y1 = y + dstY;
			if (y1 >= m_frameHeight)
				break;
			for (int x = 0; x < dstWidth; x++)
			{
				int x1 = x + dstX;
				if (x1 >= m_frameWidth)
					break;
				((uint32_t *)dstBuf)[y * dstWidth + x] = ((uint32_t *)m_frameData)[y1 * m_frameWidth + x1];
			}
		}
	}
	else if (rotation == 90)
	{
		for (int y = 0; y < dstHeight; y++)
		{
			int y1 = y + dstY;
			if (y1 >= m_frameWidth)
				break;
			for (int x = 0; x < dstWidth; x++)
			{
				int x1 = x + dstX;
				if (x1 >= m_frameHeight)
					break;
				((uint32_t *)dstBuf)[y * dstWidth + x] = ((uint32_t *)m_frameData)[x1 * m_frameWidth + (m_frameWidth - y1 - 1)];
			}
		}
	}
	else if (rotation == 180)
	{
		for (int y = 0; y < dstHeight; y++)
		{
			int y1 = y + dstY;
			if (y1 >= m_frameWidth)
				break;
			for (int x = 0; x < dstWidth; x++)
			{
				int x1 = x + dstX;
				if (x1 >= m_frameHeight)
					break;
				((uint32_t *)dstBuf)[y * dstWidth + x] = ((uint32_t *)m_frameData)[(m_frameHeight - y1 - 1) * m_frameWidth + (m_frameWidth - x1 - 1)];
			}
		}
	}
	else if (rotation == 270)
	{
		for (int y = 0; y < dstHeight; y++)
		{
			int y1 = y + dstY;
			if (y1 >= m_frameWidth)
				break;
			for (int x = 0; x < dstWidth; x++)
			{
				int x1 = x + dstX;
				if (x1 >= m_frameHeight)
					break;
				((uint32_t *)dstBuf)[y * dstWidth + x] = ((uint32_t *)m_frameData)[(m_frameHeight - x1 - 1) * m_frameWidth + y1];
			}
		}
	}
}
