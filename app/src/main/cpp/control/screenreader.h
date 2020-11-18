#ifndef _SCREENREADER_H
#define _SCREENREADER_H


class ScreenReader
{
public:
	/**
	 * 通知当前屏幕的旋转角度，以便截屏后对图像进行旋转
	 * @param rotation
	 */
	void SetRotation(int rotation);

	/**
	 * 捕获当前屏幕，固定返回32位RGBA像素数组
	 * @param x
	 * @param y
	 * @param width
	 * @param height
	 * @param outBuf 需由调用者提供足够大的输出缓冲区，可根据捕获区域大小进行分配
	 * @return
	 */
	virtual int CaptureScreen(int x, int y, int width, int height, void *outBuf);

	ScreenReader();
	virtual ~ScreenReader();

protected:
	int m_frameWidth;
	int m_frameHeight;
	int m_frameBpp;
	const void *m_frameData;
	size_t m_frameSize;

	virtual int CaptureFrame();
	virtual void ReleaseFrameBuffer();
	virtual int GetRotation();
	void CopyFrameBuffer(int dstX, int dstY, int dstWidth, int dstHeight, void *dstBuf);

private:
	void *m_frameBuf;
	int m_rotation;
};

#endif
