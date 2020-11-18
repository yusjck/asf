//
// Created by Jack on 2019/6/17.
//

#ifndef ASF_CMDDISPATCHER_H
#define ASF_CMDDISPATCHER_H


#include "ipcserver.h"
#include "cmdcontext.h"
#include "controller.h"

class CmdDispatcher : public IpcConnection
{
public:
	CmdDispatcher();
	virtual ~CmdDispatcher();

private:
	Controller *m_controller;
	ColorAssist *m_color;
	InputAssist *m_input;
	ProcessBinder *m_process;
	PluginManager *m_plugin;
	bool m_authorized;

	void CaptureBitmap(CCmdContext &_ctx);
	void ReleaseBitmap(CCmdContext &_ctx);
	void GetPreviousCapture(CCmdContext &_ctx);
	void CreateBitmap(CCmdContext &_ctx);
	void GetBitmapFile(CCmdContext &_ctx);
	void FindPicture(CCmdContext &_ctx);
	void FindColor(CCmdContext &_ctx);
	void FindColorEx(CCmdContext &_ctx);
	void GetPixelColor(CCmdContext &_ctx);
	void GetColorCount(CCmdContext &_ctx);
	void IsSimilarColor(CCmdContext &_ctx);
	void FindString(CCmdContext &_ctx);
	void FindStringEx(CCmdContext &_ctx);
	void SetOcrDict(CCmdContext &_ctx);
	void OcrExtract(CCmdContext &_ctx);
	void SimpleOcr(CCmdContext &_ctx);
	void SimpleOcrEx(CCmdContext &_ctx);

	void KeyDown(CCmdContext &_ctx);
	void KeyUp(CCmdContext &_ctx);
	void KeyPress(CCmdContext &_ctx);
	void MouseMove(CCmdContext &_ctx);
	void MouseDown(CCmdContext &_ctx);
	void MouseUp(CCmdContext &_ctx);
	void MouseClick(CCmdContext &_ctx);
	void TouchMove(CCmdContext &_ctx);
	void TouchDown(CCmdContext &_ctx);
	void TouchUp(CCmdContext &_ctx);
	void TouchTap(CCmdContext &_ctx);
	void TouchSwipe(CCmdContext &_ctx);
	void SendString(CCmdContext &_ctx);

	void BindProcess(CCmdContext &_ctx);
	void UnbindWindow(CCmdContext &_ctx);

	void SearchCode(CCmdContext &_ctx);
	void ReadMemory(CCmdContext &_ctx);
	void WriteMemory(CCmdContext &_ctx);
	void LoadDllExtend(CCmdContext &_ctx);
	void FreeDllExtend(CCmdContext &_ctx);
	void InvokeDllExtend(CCmdContext &_ctx);

	void RunApp(CCmdContext &_ctx);
	void KillApp(CCmdContext &_ctx);
	void GetProcessId(CCmdContext &_ctx);
	void SendDeviceCmd(CCmdContext &_ctx);
	void GetDeviceInfo(CCmdContext &_ctx);

	void InitMicroCore(CCmdContext &_ctx);
	void GetMicroCoreVersion(CCmdContext &_ctx);
	void RequestControl(CCmdContext &_ctx);
	void GetPluginCrc(CCmdContext &_ctx);
	void PushPlugin(CCmdContext &_ctx);
	void LoadPlugins(CCmdContext &_ctx);
	void GetPluginCommands(CCmdContext &_ctx);
	void InvokePluginCommand(CCmdContext &_ctx);
	void DestroyPluginObject(CCmdContext &_ctx);
	void CleanupPlugins(CCmdContext &_ctx);

	void OnCmdDispatch(CCmdContext &_ctx);
	virtual void OnReceive(CBuffer &pack);
};


#endif //ASF_CMDDISPATCHER_H
