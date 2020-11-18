//
// Created by Jack on 2019/7/21.
//

#ifndef ASF_CONTROLLER_H
#define ASF_CONTROLLER_H


#include <memory>
#include <object.h>
#include "colorassist.h"
#include "inputassist.h"
#include "deviceinfo.h"
#include "pluginmgr.h"
#include "processbinder.h"
#include "androidcaller.h"

class Controller : public CRefObject
{
public:
	static Controller *NewInstance();
	int RunApp(const char *app, const char *parameter, uint32_t reserve, bool waitAppExit, uint32_t &pidOrExitCode);
	int KillApp(uint32_t pid, const char *processName);
	int GetProcessId(const char *name, std::string &result);
	int SendDeviceCmd(const char *cmd, const char *args, std::string &result);
	int GetDeviceInfo(const char *infoClass, std::string &result);

	AndroidCaller m_androidCaller;
	ColorAssist m_color;
	InputAssist m_input;
	ProcessBinder m_process;
	PluginManager m_plugin;

private:
	Controller();
	virtual ~Controller();

private:
	std::shared_ptr<DeviceInfo> m_deviceInfo;
};


#endif //ASF_CONTROLLER_H
