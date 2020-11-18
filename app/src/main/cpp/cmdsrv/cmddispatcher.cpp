//
// Created by Jack on 2019/6/17.
//

#include "common.h"
#include "cmdcode.h"
#include "cmddispatcher.h"

void CmdDispatcher::CaptureBitmap(CCmdContext &_ctx)
{
	PB_GET_VALUES_TYPE4(int, int, int, int);
	handle_t bitmapHandle = nullptr;
	int ret = m_color->CaptureBitmap(a1, a2, a3, a4, bitmapHandle);
	_ctx.InitRetContext(ret);
	PB_WRITE_VALUES1(bitmapHandle);
}

void CmdDispatcher::ReleaseBitmap(CCmdContext &_ctx)
{
	PB_GET_VALUES_TYPE1(handle_t);
	int ret = m_color->ReleaseBitmap(a1);
	_ctx.InitRetContext(ret);
}

void CmdDispatcher::GetPreviousCapture(CCmdContext &_ctx)
{
	handle_t bitmapHandle = nullptr;
	int ret = m_color->GetPreviousCapture(bitmapHandle);
	_ctx.InitRetContext(ret);
	PB_WRITE_VALUES1(bitmapHandle);
}

void CmdDispatcher::CreateBitmap(CCmdContext &_ctx)
{
	uint32_t imgFileSize;
	const void *imgFile = _ctx.GetBin(1, imgFileSize);
	handle_t bitmapHandle = nullptr;
	int ret = m_color->CreateBitmap(imgFile, imgFileSize, bitmapHandle);
	_ctx.InitRetContext(ret);
	PB_WRITE_VALUES1(bitmapHandle);
}

void CmdDispatcher::GetBitmapFile(CCmdContext &_ctx)
{
	PB_GET_VALUES_TYPE5(handle_t, int, int, int, int);
	CBuffer file;
	int ret = m_color->CaptureBitmapToPngFile(a1, a2, a3, a4, a5, file);
	_ctx.InitRetContext(ret);
	PB_WRITE_VALUES1(file);
}

void CmdDispatcher::FindPicture(CCmdContext &_ctx)
{
	PB_GET_VALUES_TYPE9(handle_t, int, int, int, int, handle_t, const char *, float, int);
	std::string strResult;
	int ret = m_color->FindPicture(a1, a2, a3, a4, a5, a6, a7, a8, a9, strResult);
	_ctx.InitRetContext(ret);
	PB_WRITE_VALUES1(strResult.c_str());
}

void CmdDispatcher::FindColor(CCmdContext &_ctx)
{
	PB_GET_VALUES_TYPE8(handle_t, int, int, int, int, const char *, float, int);
	std::string strResult;
	int ret = m_color->FindColor(a1, a2, a3, a4, a5, a6, a7, a8, strResult);
	_ctx.InitRetContext(ret);
	PB_WRITE_VALUES1(strResult.c_str());
}

void CmdDispatcher::FindColorEx(CCmdContext &_ctx)
{
	PB_GET_VALUES_TYPE9(handle_t, int, int, int, int, const char *, float, int, int);
	std::string strResult;
	int ret = m_color->FindColorEx(a1, a2, a3, a4, a5, a6, a7, a8, a9, strResult);
	_ctx.InitRetContext(ret);
	PB_WRITE_VALUES1(strResult.c_str());
}

void CmdDispatcher::GetPixelColor(CCmdContext &_ctx)
{
	PB_GET_VALUES_TYPE3(handle_t, int, int);
	std::string strResult;
	int ret = m_color->GetPixelColor(a1, a2, a3, strResult);
	_ctx.InitRetContext(ret);
	PB_WRITE_VALUES1(strResult.c_str());
}

void CmdDispatcher::GetColorCount(CCmdContext &_ctx)
{
	PB_GET_VALUES_TYPE7(handle_t, int, int, int, int, const char *, float);
	int colorCount;
	int ret = m_color->GetColorCount(a1, a2, a3, a4, a5, a6, a7, colorCount);
	_ctx.InitRetContext(ret);
	PB_WRITE_VALUES1(colorCount);
}

void CmdDispatcher::IsSimilarColor(CCmdContext &_ctx)
{
	PB_GET_VALUES_TYPE5(handle_t, int, int, const char *, float);
	bool isSimilar = false;
	int ret = m_color->IsSimilarColor(a1, a2, a3, a4, a5, isSimilar);
	_ctx.InitRetContext(ret);
	PB_WRITE_VALUES1(isSimilar);
}

void CmdDispatcher::FindString(CCmdContext &_ctx)
{
	PB_GET_VALUES_TYPE8(handle_t, int, int, int, int, const char *, const char *, int);
	std::string strResult;
	int ret = m_color->m_simpleOcr.FindString(a1, a2, a3, a4, a5, a6, a7, a8, strResult);
	_ctx.InitRetContext(ret);
	PB_WRITE_VALUES1(strResult.c_str());
}

void CmdDispatcher::FindStringEx(CCmdContext &_ctx)
{
	PB_GET_VALUES_TYPE10(handle_t, int, int, int, int, const char *, const char *, const char *, float, int);
	std::string strResult;
	int ret = m_color->m_simpleOcr.FindStringEx(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, strResult);
	_ctx.InitRetContext(ret);
	PB_WRITE_VALUES1(strResult.c_str());
}

void CmdDispatcher::SetOcrDict(CCmdContext &_ctx)
{
	PB_GET_VALUES_TYPE4(const char *, int, const char *, const char *);
	int ret = m_color->m_simpleOcr.SetOcrDict(a1, a2, a3);
	_ctx.InitRetContext(ret);
}

void CmdDispatcher::OcrExtract(CCmdContext &_ctx)
{
	PB_GET_VALUES_TYPE6(handle_t, int, int, int, int, const char *);
	std::string strResult;
	int ret = m_color->m_simpleOcr.OcrExtract(a1, a2, a3, a4, a5, a6, strResult);
	_ctx.InitRetContext(ret);
	PB_WRITE_VALUES1(strResult.c_str());
}

void CmdDispatcher::SimpleOcr(CCmdContext &_ctx)
{
	PB_GET_VALUES_TYPE8(handle_t, int, int, int, int, const char *, const char *, float);
	std::string strResult;
	int ret = m_color->m_simpleOcr.Ocr(a1, a2, a3, a4, a5, a6, a7, a8, strResult);
	_ctx.InitRetContext(ret);
	PB_WRITE_VALUES1(strResult.c_str());
}

void CmdDispatcher::SimpleOcrEx(CCmdContext &_ctx)
{
	PB_GET_VALUES_TYPE9(handle_t, int, int, int, int, const char *, const char *, float, bool);
	std::string strResult;
	int ret = m_color->m_simpleOcr.OcrEx(a1, a2, a3, a4, a5, a6, a7, a8, a9, strResult);
	_ctx.InitRetContext(ret);
	PB_WRITE_VALUES1(strResult.c_str());
}

void CmdDispatcher::KeyDown(CCmdContext &_ctx)
{
	PB_GET_VALUES_TYPE1(int);
	int ret = m_input->KeyDown(a1);
	_ctx.InitRetContext(ret);
}

void CmdDispatcher::KeyUp(CCmdContext &_ctx)
{
	PB_GET_VALUES_TYPE1(int);
	int ret = m_input->KeyUp(a1);
	_ctx.InitRetContext(ret);
}

void CmdDispatcher::KeyPress(CCmdContext &_ctx)
{
	PB_GET_VALUES_TYPE2(int, uint32_t);
	int ret = m_input->KeyPress(a1, a2);
	_ctx.InitRetContext(ret);
}

void CmdDispatcher::MouseMove(CCmdContext &_ctx)
{
	PB_GET_VALUES_TYPE3(int, int, uint32_t);
	int ret = m_input->MouseMove(a1, a2, a3);
	_ctx.InitRetContext(ret);
}

void CmdDispatcher::MouseDown(CCmdContext &_ctx)
{
	PB_GET_VALUES_TYPE1(int);
	int ret = m_input->MouseDown(a1);
	_ctx.InitRetContext(ret);
}

void CmdDispatcher::MouseUp(CCmdContext &_ctx)
{
	PB_GET_VALUES_TYPE1(int);
	int ret = m_input->MouseUp(a1);
	_ctx.InitRetContext(ret);
}

void CmdDispatcher::MouseClick(CCmdContext &_ctx)
{
	PB_GET_VALUES_TYPE2(int, uint32_t);
	int ret = m_input->MouseClick(a1, a2);
	_ctx.InitRetContext(ret);
}

void CmdDispatcher::TouchMove(CCmdContext &_ctx)
{
	PB_GET_VALUES_TYPE4(int, int, int, uint32_t);
	int ret = m_input->TouchMove(a1, a2, a3, a4);
	_ctx.InitRetContext(ret);
}

void CmdDispatcher::TouchDown(CCmdContext &_ctx)
{
	PB_GET_VALUES_TYPE3(int, int, int);
	int ret = m_input->TouchDown(a1, a2, a3);
	_ctx.InitRetContext(ret);
}

void CmdDispatcher::TouchUp(CCmdContext &_ctx)
{
	PB_GET_VALUES_TYPE1(int);
	int ret = m_input->TouchUp(a1);
	_ctx.InitRetContext(ret);
}

void CmdDispatcher::TouchTap(CCmdContext &_ctx)
{
	PB_GET_VALUES_TYPE3(int, int, int);
	int ret = m_input->TouchTap(a1, a2, a3);
	_ctx.InitRetContext(ret);
}

void CmdDispatcher::TouchSwipe(CCmdContext &_ctx)
{
	PB_GET_VALUES_TYPE6(int, int, int, int, int, uint32_t);
	int ret = m_input->TouchSwipe(a1, a2, a3, a4, a5, a6);
	_ctx.InitRetContext(ret);
}

void CmdDispatcher::SendString(CCmdContext &_ctx)
{
	PB_GET_VALUES_TYPE2(uint32_t, const char *);
	int ret = m_input->SendString(a1, a2);
	_ctx.InitRetContext(ret);
}

void CmdDispatcher::BindProcess(CCmdContext &_ctx)
{
	PB_GET_VALUES_TYPE2(uint32_t, uint32_t);
	_ctx.InitRetContext(m_process->BindProcess(a1, a2));
}

void CmdDispatcher::UnbindWindow(CCmdContext &_ctx)
{
	_ctx.InitRetContext(m_process->UnbindProcess());
}

void CmdDispatcher::SearchCode(CCmdContext &_ctx)
{
	PB_GET_VALUES_TYPE3(const char *, const char *, const char *);
	std::string strResult;
	int ret = m_process->SearchCode(a1, a2, a3, strResult);
	_ctx.InitRetContext(ret);
	PB_WRITE_VALUES1(strResult.c_str());
}

void CmdDispatcher::ReadMemory(CCmdContext &_ctx)
{
	PB_GET_VALUES_TYPE2(const char *, uint32_t);
	CBuffer readbuf;
	void *buf = readbuf.GetBuffer(a2);
	if (buf == NULL)
	{
		_ctx.InitRetContext(ERR_ALLOC_MEMORY_FAILED);
		return;
	}

	int ret = m_process->Read(a1, buf, a2);
	if (!IS_SUCCESS(ret))
	{
		_ctx.InitRetContext(ret);
		return;
	}

	_ctx.InitRetContext(ret);
	_ctx.WriteBin(buf, a2);
}

void CmdDispatcher::WriteMemory(CCmdContext &_ctx)
{
	PB_GET_VALUES_TYPE1(const char *);
	uint32_t len;
	const void *p = _ctx.GetBin(2, len);
	int ret = m_process->Write(a1, p, len);
	_ctx.InitRetContext(ret);
	PB_WRITE_VALUES1(len);
}

void CmdDispatcher::LoadDllExtend(CCmdContext &_ctx)
{
	uint32_t dllSize;
	const void *dllExtend = _ctx.GetBin(1, dllSize);
	uint32_t dllExtendIndex = 0;
	int ret = m_process->LoadExtModule(dllExtend, dllSize, dllExtendIndex);
	_ctx.InitRetContext(ret);
	PB_WRITE_VALUES1(dllExtendIndex);
}

void CmdDispatcher::FreeDllExtend(CCmdContext &_ctx)
{
	PB_GET_VALUES_TYPE1(uint32_t);
	int ret = m_process->FreeExtModule(a1);
	_ctx.InitRetContext(ret);
}

void CmdDispatcher::InvokeDllExtend(CCmdContext &_ctx)
{
	PB_GET_VALUES_TYPE4(uint32_t, const char *, const char *, uint32_t);
	std::string strResult;
	int ret = m_process->InvokeExtModule(a1, a2, a3, a4, strResult);
	_ctx.InitRetContext(ret);
	PB_WRITE_VALUES1(strResult.c_str());
}

void CmdDispatcher::RunApp(CCmdContext &_ctx)
{
	PB_GET_VALUES_TYPE4(const char *, const char *, uint32_t, bool);
	uint32_t processIdOrExitCode;
	_ctx.InitRetContext(m_controller->RunApp(a1, a2, a3, a4, processIdOrExitCode));
	_ctx.Write(processIdOrExitCode);
}

void CmdDispatcher::KillApp(CCmdContext &_ctx)
{
	PB_GET_VALUES_TYPE2(uint32_t, const char *);
	_ctx.InitRetContext(m_controller->KillApp(a1, a2));
}

void CmdDispatcher::GetProcessId(CCmdContext &_ctx)
{
	const char *name = _ctx.GetStr(1);
	std::string result;
	_ctx.InitRetContext(m_controller->GetProcessId(name, result));
	_ctx.Write(result);
}

void CmdDispatcher::SendDeviceCmd(CCmdContext &_ctx)
{
	const char *cmd = _ctx.GetStr(1);
	const char *args = _ctx.GetStr(2);
	std::string result;
	_ctx.InitRetContext(m_controller->SendDeviceCmd(cmd, args, result));
	_ctx.Write(result);
}

void CmdDispatcher::GetDeviceInfo(CCmdContext &_ctx)
{
	const char *infoClass = _ctx.GetStr(1);
	std::string result;
	_ctx.InitRetContext(m_controller->GetDeviceInfo(infoClass, result));
	_ctx.Write(result);
}

void CmdDispatcher::GetMicroCoreVersion(CCmdContext &_ctx)
{
	_ctx.InitRetContext(ERR_NONE);
	_ctx.WriteInt(MICRO_CORE_VERSION);
	_ctx.WriteUint(VERF_DISABLE_UPDATE | VERF_RUN_ON_ANDROID);
}

void CmdDispatcher::RequestControl(CCmdContext &_ctx)
{
	const char *deviceName = _ctx.GetStr(1);
	const char *deviceSignature = _ctx.GetStr(2);
	LOGI("%s connect to cmdserver", deviceName);
	int ret = m_controller->m_androidCaller.RequestControl(deviceName, deviceSignature);
	if (IS_SUCCESS(ret))
		m_authorized = true;
	_ctx.InitRetContext(ret);
}

void CmdDispatcher::InitMicroCore(CCmdContext &_ctx)
{
	_ctx.InitRetContext(ERR_NONE);
}

void CmdDispatcher::GetPluginCrc(CCmdContext &_ctx)
{
	const char *fileName = _ctx.GetStr(1);
	std::string strPath = format("%s/%s", m_controller->m_plugin.GetDefaultPluginDir().c_str(), fileName);
	uint32_t pluginCrc;
	if (!CalculateFileCrc32(strPath.c_str(), &pluginCrc))
	{
		_ctx.InitRetContext(ERR_OPEN_FILE_FAILED);
		PB_WRITE_VALUES1((uint32_t)0);
	}
	else
	{
		_ctx.InitRetContext(ERR_NONE);
		PB_WRITE_VALUES1(pluginCrc);
	}
}

void CmdDispatcher::PushPlugin(CCmdContext &_ctx)
{
	const char *fileName = _ctx.GetStr(1);
	uint32_t fileSize;
	const void *pFileStream = _ctx.GetBin(2, fileSize);
	system(format("mkdir %s", m_controller->m_plugin.GetDefaultPluginDir().c_str()).c_str());
	std::string strPath = format("%s/%s", m_controller->m_plugin.GetDefaultPluginDir().c_str(), fileName);
	if (WriteDataToFile(strPath.c_str(), pFileStream, fileSize, false))
		_ctx.InitRetContext(ERR_NONE);
	else
		_ctx.InitRetContext(ERR_OPEN_FILE_FAILED);
}

void CmdDispatcher::LoadPlugins(CCmdContext &_ctx)
{
	uint32_t scriptId = _ctx.GetUint(1);
	m_plugin->LoadPlugins(NULL, scriptId, true);
	_ctx.InitRetContext(ERR_NONE);
}

void CmdDispatcher::GetPluginCommands(CCmdContext &_ctx)
{
	std::vector<std::string> cmds;
	bool ret = m_plugin->GetCommandList(cmds, PluginManager::REMOTE_CMD);
	if (!ret)
	{
		_ctx.InitRetContext(ERR_INVOKE_FAILED);
		return;
	}
	std::string str;
	for (int i = 0; i < cmds.size(); i++)
	{
		if (str.empty())
			str = cmds[i];
		else
			str += "|" + cmds[i];
	}
	_ctx.InitRetContext(ERR_NONE);
	_ctx.WriteStr(str.c_str());
}

void CmdDispatcher::InvokePluginCommand(CCmdContext &_ctx)
{
	const char *commandName = _ctx.GetStr(1);
	uint32_t invokeDataSize;
	const void *invokeData = _ctx.GetBin(2, invokeDataSize);
	CBuffer retbuf;
	bool ret = m_plugin->InvokePluginCommand(commandName, invokeData, invokeDataSize, retbuf);
	if (!ret)
	{
		_ctx.InitRetContext(ERR_INVOKE_FAILED);
		return;
	}
	_ctx.InitRetContext(ERR_NONE);
	_ctx.WriteBin(retbuf.GetBuffer(), (uint32_t)retbuf.GetSize());
}

void CmdDispatcher::DestroyPluginObject(CCmdContext &_ctx)
{
	PB_GET_VALUES_TYPE2(const char *, const void *);
	m_plugin->DestroyPluginObject(a1, (void *)a2);
	_ctx.InitRetContext(ERR_NONE);
}

void CmdDispatcher::CleanupPlugins(CCmdContext &_ctx)
{
	const char *reservedList = _ctx.GetStr(1);
	m_plugin->CleanupPlugins(NULL, reservedList);
	_ctx.InitRetContext(ERR_NONE);
}

CmdDispatcher::CmdDispatcher()
{
	LOGI("new instance CmdDispatcher\n");
	m_controller = nullptr;
	m_authorized = false;
}

CmdDispatcher::~CmdDispatcher()
{
	LOGI("destroy instance CmdDispatcher\n");
	if (m_controller)
		m_controller->Release();
}

#define COMMAND_MAP_BEGIN switch (cmdCode) {
#define COMMAND_MAP_END default:\
		_ctx.InitRetContext(ERR_INVALID_INVOKE);\
		break;\
	}
#define ON_COMMAND(cmd, handler)\
		case cmd:\
			handler(_ctx);\
			break;

void CmdDispatcher::OnCmdDispatch(CCmdContext &_ctx)
{
	int cmdCode = _ctx.GetCmdCode();
	LOGI("receive cmd=%d\n", cmdCode);

	if (cmdCode == MCMD_GET_MICRO_CORE_VERSION)
	{
		GetMicroCoreVersion(_ctx);
		return;
	}

	if (cmdCode == MCMD_REQUEST_CONTROL)
	{
		RequestControl(_ctx);
		return;
	}

	if (!m_authorized)
	{
		_ctx.InitRetContext(ERR_ACCESS_DENIED);
		return;
	}

	COMMAND_MAP_BEGIN
		ON_COMMAND(MCMD_KEY_DOWN, KeyDown)
		ON_COMMAND(MCMD_KEY_UP, KeyUp)
		ON_COMMAND(MCMD_KEY_PRESS, KeyPress)
		ON_COMMAND(MCMD_MOUSE_MOVE, MouseMove)
		ON_COMMAND(MCMD_MOUSE_DOWN, MouseDown)
		ON_COMMAND(MCMD_MOUSE_UP, MouseUp)
		ON_COMMAND(MCMD_MOUSE_CLICK, MouseClick)
		ON_COMMAND(MCMD_TOUCH_MOVE, TouchMove)
		ON_COMMAND(MCMD_TOUCH_DOWN, TouchDown)
		ON_COMMAND(MCMD_TOUCH_UP, TouchUp)
		ON_COMMAND(MCMD_TOUCH_TAP, TouchTap)
		ON_COMMAND(MCMD_TOUCH_SWIPE, TouchSwipe)
		ON_COMMAND(MCMD_SEND_STRING, SendString)

		ON_COMMAND(MCMD_CAPTURE_BITMAP, CaptureBitmap)
		ON_COMMAND(MCMD_RELEASE_BITMAP, ReleaseBitmap)
		ON_COMMAND(MCMD_GET_PREVIOUS_CAPTURE, GetPreviousCapture)
		ON_COMMAND(MCMD_CREATE_BITMAP, CreateBitmap)
		ON_COMMAND(MCMD_GET_BITMAP_FILE, GetBitmapFile)
		ON_COMMAND(MCMD_FIND_PICTURE, FindPicture)
		ON_COMMAND(MCMD_FIND_COLOR, FindColor)
		ON_COMMAND(MCMD_FIND_COLOR_EX, FindColorEx)
		ON_COMMAND(MCMD_GET_PIXEL_COLOR, GetPixelColor)
		ON_COMMAND(MCMD_GET_COLOR_COUNT, GetColorCount)
		ON_COMMAND(MCMD_IS_SIMILAR_COLOR, IsSimilarColor)

		ON_COMMAND(MCMD_FIND_STRING, FindString)
		ON_COMMAND(MCMD_FIND_STRING_EX, FindStringEx)
		ON_COMMAND(MCMD_SET_OCR_DICT, SetOcrDict)
		ON_COMMAND(MCMD_OCR_EXTRACT, OcrExtract)
		ON_COMMAND(MCMD_SIMPLE_OCR, SimpleOcr)
		ON_COMMAND(MCMD_SIMPLE_OCR_EX, SimpleOcrEx)

		ON_COMMAND(MCMD_BIND_PROCESS, BindProcess)
		ON_COMMAND(MCMD_UNBIND_WINDOW, UnbindWindow)

		ON_COMMAND(MCMD_SEARCH_CODE, SearchCode)
		ON_COMMAND(MCMD_READ_MEMORY, ReadMemory)
		ON_COMMAND(MCMD_WRITE_MEMORY, WriteMemory)
		ON_COMMAND(MCMD_LOAD_DLL_EXTEND, LoadDllExtend)
		ON_COMMAND(MCMD_FREE_DLL_EXTEND, FreeDllExtend)
		ON_COMMAND(MCMD_INVOKE_DLL_EXTEND, InvokeDllExtend)

//		ON_COMMAND(MCMD_GET_MICRO_CORE_VERSION, GetMicroCoreVersion)
//		ON_COMMAND(MCMD_REQUEST_CONTROL, RequestControl)
		ON_COMMAND(MCMD_INIT_MICRO_CORE, InitMicroCore)
		ON_COMMAND(MCMD_GET_PLUGIN_CRC, GetPluginCrc)
		ON_COMMAND(MCMD_PUSH_PLUGIN, PushPlugin)
		ON_COMMAND(MCMD_LOAD_PLUGINS, LoadPlugins)
		ON_COMMAND(MCMD_GET_PLUGIN_COMMANDS, GetPluginCommands)
		ON_COMMAND(MCMD_INVOKE_PLUGIN_COMMAND, InvokePluginCommand)
		ON_COMMAND(MCMD_DESTROY_PLUGIN_OBJECT, DestroyPluginObject)
		ON_COMMAND(MCMD_CLEANUP_PLUGINS, CleanupPlugins)

		ON_COMMAND(MCMD_RUN_APP, RunApp)
		ON_COMMAND(MCMD_KILL_APP, KillApp)
		ON_COMMAND(MCMD_GET_PROCESS_ID, GetProcessId)
		ON_COMMAND(MCMD_SEND_DEVICE_CMD, SendDeviceCmd)
		ON_COMMAND(MCMD_GET_DEVICE_INFO, GetDeviceInfo)
	COMMAND_MAP_END
}

void CmdDispatcher::OnReceive(CBuffer &pack)
{
	CCmdContext _ctx;
	if (!_ctx.ParseCmdContext(pack))
	{
		Disconnect();
		return;
	}

	if (m_controller == nullptr)
	{
		m_controller = Controller::NewInstance();
		if (m_controller == nullptr)
		{
			LOGE("new CmdDispatcher failed\n");
			Disconnect();
			return;
		}

		m_color = &m_controller->m_color;
		m_input = &m_controller->m_input;
		m_process = &m_controller->m_process;
		m_plugin = &m_controller->m_plugin;
	}

	try
	{
		OnCmdDispatch(_ctx);
	}
	catch (std::string s)
	{
		LOGE("except in dispatch_cmd, %s", s.c_str());
		Disconnect();
		return;
	}
	catch (...)
	{
		Disconnect();
		return;
	}

	Send(_ctx.GetRetCtx());
}
