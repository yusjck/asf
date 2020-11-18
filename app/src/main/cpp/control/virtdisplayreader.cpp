#include <unistd.h>
#include <cmath>
#include <condition_variable>
#include <chrono>
#include <dlfcn.h>

#include "common.h"
#include "Minicap.hpp"
#include "virtdisplayreader.h"

class FrameWaiter : public Minicap::FrameAvailableListener {
public:
	FrameWaiter()
			: mPendingFrames(0),
			  mTimeout(std::chrono::milliseconds(100)),
			  mStopped(false) {
	}

	int
	waitForFrame() {
		std::unique_lock<std::mutex> lock(mMutex);

		while (!mStopped) {
			if (mCondition.wait_for(lock, mTimeout, [this] {return mPendingFrames > 0; })) {
				return mPendingFrames--;
			}
		}

		return 0;
	}

	void
	reportExtraConsumption(int count) {
		std::unique_lock<std::mutex> lock(mMutex);
		mPendingFrames -= count;
	}

	void
	onFrameAvailable() {
		std::unique_lock<std::mutex> lock(mMutex);
		mPendingFrames += 1;
		mCondition.notify_one();
	}

	void
	stop() {
		mStopped = true;
	}

	bool
	isStopped() {
		return mStopped;
	}

private:
	std::mutex mMutex;
	std::condition_variable mCondition;
	std::chrono::milliseconds mTimeout;
	int mPendingFrames;
	bool mStopped;
};

static void *gMinicapLib = nullptr;
static minicap_try_get_display_info_t minicap_try_get_display_info;
static minicap_create_t minicap_create;
static minicap_free_t minicap_free;
static minicap_start_thread_pool_t minicap_start_thread_pool;

static FrameWaiter gWaiter;
static Minicap* minicap = nullptr;
static pthread_mutex_t gLock;
static pthread_t gThread = 0;
static CBuffer gScrBuf;
static Minicap::Frame gFrame;

static bool minicap_init()
{
	if (gMinicapLib == nullptr)
	{
		// 加载工作目录下的minicap.so
		char soPath[MAX_PATH];
		getcwd(soPath, sizeof(soPath));
		strcat(soPath, "/minicap.so");
		gMinicapLib = dlopen(soPath, RTLD_NOW);
		if (!gMinicapLib)
		{
			LOGI("%s\n", dlerror());
			return false;
		}

		*(void **)&minicap_try_get_display_info = dlsym(gMinicapLib, "_Z28minicap_try_get_display_infoiPN7Minicap11DisplayInfoE");
		*(void **)&minicap_create = dlsym(gMinicapLib, "_Z14minicap_createi");
		*(void **)&minicap_free = dlsym(gMinicapLib, "_Z12minicap_freeP7Minicap");
		*(void **)&minicap_start_thread_pool = dlsym(gMinicapLib, "_Z25minicap_start_thread_poolv");
		LOGI("minicap_init() ok\n");
	}
	return true;
}

static bool minicap_enabled()
{
	return gThread != 0;
}

void *VirtDisplayReader::FrameRefreshThread(void *arg)
{
	while (!gWaiter.isStopped())
	{
		int pending = gWaiter.waitForFrame();
		if (!pending)
		{
			LOGI("Unable to wait for frame\n");
			return nullptr;
		}

		Minicap::Frame frame{};
		if (pending > 1)
		{
			gWaiter.reportExtraConsumption(pending - 1);
			while (--pending >= 1)
			{
				if (minicap->consumePendingFrame(&frame) != 0)
				{
					LOGI("Unable to consume pending frame\n");
					return nullptr;
				}
				minicap->releaseConsumedFrame(&frame);
			}
		}

		if (minicap->consumePendingFrame(&frame) != 0)
		{
			LOGI("Unable to consume pending frame\n");
			return nullptr;
		}

		pthread_mutex_lock(&gLock);
		size_t size = frame.width * frame.height * 4;
		auto *p = (unsigned char *)gScrBuf.GetBuffer(size);
		for (int y = 0; y < frame.height; y++)
		{
			for (int x = 0; x < frame.width; x++)
			{
				int si = (y * frame.stride + x);
				int di = (y * frame.width + x);
//				((unsigned char *)p)[di + 2] = ((unsigned char *)frame.data)[si + 0];
//				((unsigned char *)p)[di + 1] = ((unsigned char *)frame.data)[si + 1];
//				((unsigned char *)p)[di + 0] = ((unsigned char *)frame.data)[si + 2];
//				((unsigned char *)p)[di + 3] = ((unsigned char *)frame.data)[si + 3];
				((unsigned int *)p)[di] = ((unsigned int *)frame.data)[si];
			}
		}
		gFrame = frame;
		gFrame.data = gScrBuf.GetBuffer();
		pthread_mutex_unlock(&gLock);
		minicap->releaseConsumedFrame(&frame);

		usleep(1000 * 50);
	}
	return nullptr;
}

int VirtDisplayReader::InitVirtDisplay(int width, int height)
{
	if (!minicap_init())
		return ERR_INVOKE_FAILED;

	// Start Android's thread pool so that it will be able to serve our requests.
	minicap_start_thread_pool();

	// Set real display size.
	Minicap::DisplayInfo realInfo = {0};
	realInfo.width = static_cast<uint32_t>(width);
	realInfo.height = static_cast<uint32_t>(height);

	// Figure out desired display size.
	Minicap::DisplayInfo desiredInfo = {0};
	desiredInfo.width = static_cast<uint32_t>(width);
	desiredInfo.height = static_cast<uint32_t>(height);

	// Set up minicap.
	minicap = minicap_create(0);
	if (minicap == nullptr)
	{
		LOGI("minicap_create() failed\n");
		return ERR_INVOKE_FAILED;
	}

	if (minicap->setRealInfo(realInfo) != 0)
	{
		LOGI("Minicap did not accept real display info\n");
		return ERR_INVOKE_FAILED;
	}

	if (minicap->setDesiredInfo(desiredInfo) != 0)
	{
		LOGI("Minicap did not accept desired display info\n");
		return ERR_INVOKE_FAILED;
	}

	minicap->setFrameAvailableListener(&gWaiter);

	if (minicap->applyConfigChanges() != 0)
	{
		LOGI("Unable to start minicap with current config\n");
		return ERR_INVOKE_FAILED;
	}

	pthread_mutex_init(&gLock, nullptr);
	pthread_create(&gThread, nullptr, FrameRefreshThread, nullptr);
	LOGI("InitVirtDisplay() ok\n");
	return ERR_NONE;
}

void VirtDisplayReader::UninitVirtDisplay()
{
	gWaiter.stop();
	if (gThread)
	{
		pthread_join(gThread, nullptr);
		gThread = 0;
	}
	pthread_mutex_destroy(&gLock);
	minicap_free(minicap);
}

int VirtDisplayReader::GetScreenInfo(int &width, int &height)
{
	if (!minicap_init())
		return ERR_INVOKE_FAILED;

	Minicap::DisplayInfo info{};
	if (minicap_try_get_display_info(0, &info) != 0)
		return ERR_INVOKE_FAILED;

	width = info.width;
	height = info.height;
	return ERR_NONE;
}

VirtDisplayReader::VirtDisplayReader()
{
	if (!minicap_enabled())
	{
		int screenWidth, screenHeight;
		int res = GetScreenInfo(screenWidth, screenHeight);
		if (!IS_SUCCESS(res))
			return;

		InitVirtDisplay(screenWidth, screenHeight);
	}
}

VirtDisplayReader::~VirtDisplayReader()
= default;

int VirtDisplayReader::CaptureScreen(int x, int y, int width, int height, void *outBuf)
{
	if (!minicap_enabled())
		return ERR_INVOKE_FAILED;
	return ScreenReader::CaptureScreen(x, y, width, height, outBuf);
}

int VirtDisplayReader::CaptureFrame()
{
	pthread_mutex_lock(&gLock);
	m_frameWidth = gFrame.width;
	m_frameHeight = gFrame.height;
	m_frameBpp = gFrame.bpp;
	m_frameData = gFrame.data;
	m_frameSize = gFrame.size;
	return ERR_NONE;
}

void VirtDisplayReader::ReleaseFrameBuffer()
{
	pthread_mutex_unlock(&gLock);
	m_frameWidth = 0;
	m_frameHeight = 0;
	m_frameBpp = 0;
	m_frameData = nullptr;
	m_frameSize = 0;
}

int VirtDisplayReader::GetRotation()
{
	return 0;
}
