//
// Created by Jack on 2019/7/21.
//

#include <unistd.h>
#include <sys/system_properties.h>
#include "common.h"
#include "controller.h"
#include "../base/errcode.h"
#include "inject.h"

Controller::Controller()
{
	m_deviceInfo = std::make_shared<DeviceInfo>(this);
	m_color.m_deviceInfo = m_deviceInfo;
	m_input.m_deviceInfo = m_deviceInfo;
}

Controller::~Controller()
{
	m_androidCaller.Disconnect();
}

Controller *Controller::NewInstance()
{
	Controller *inst = new Controller();
	if (!inst->m_androidCaller.TryConnect())
	{
		inst->Release();
		return NULL;
	}
	return inst;
}

int Controller::RunApp(const char *app, const char *parameter, uint32_t reserve, bool waitAppExit, uint32_t &pidOrExitCode)
{
	std::string cmd = format("%s %s", app, parameter);
	pidOrExitCode = (uint32_t)system(cmd.c_str());
	return ERR_NONE;
}

int Controller::KillApp(uint32_t pid, const char *processName)
{
	char cmd[1000] = {0};
	sprintf(cmd, "killall %s", processName);
	system(cmd);
	return ERR_NONE;
}

int Controller::GetProcessId(const char *name, std::string &result)
{
	pid_t target_pid = find_pid_of(name);
	if (target_pid == -1)
		return ERR_INVOKE_FAILED;

	result = format("%d", target_pid);
	return ERR_NONE;
}

int Controller::SendDeviceCmd(const char *cmd, const char *args, std::string &result)
{
	if (strcasecmp(cmd, "ShowMessage") == 0)
	{
		m_androidCaller.ShowMessage(args);
	}
	else if (strcasecmp(cmd, "Vibrate") == 0)
	{
		m_androidCaller.Vibrate(atoi(args));
	}
	else if (strcasecmp(cmd, "CaptureMode") == 0 || strcasecmp(cmd, "SetCaptureMode") == 0)
	{
		if (strcasecmp(args, "minicap") == 0)
		{
			m_deviceInfo->SetCaptureMode(1);
		}
		else if (strcasecmp(args, "fb") == 0)
		{
			m_deviceInfo->SetCaptureMode(2);
		}
		else if (strcasecmp(args, "sclib") == 0)
		{
			m_deviceInfo->SetCaptureMode(3);
		}
	}
	else
	{
		CallerContext ctx(ACCMD_CALLANDROID);
		ctx.WriteStr(cmd);
		ctx.WriteStr(args);
		int ret = m_androidCaller.Call(ctx);
		if (!IS_SUCCESS(ret))
			return ret;
		result = ctx.ReadStr();
	}
	return ERR_NONE;
}

int Controller::GetDeviceInfo(const char *infoClass, std::string &result)
{
	int iRet = ERR_NONE;
	if (strcasecmp(infoClass, "DisplayInfo") == 0)
	{
		int width, height, rotation;
		iRet = m_deviceInfo->GetDisplayInfo(width, height, rotation);
		if (IS_SUCCESS(iRet))
		{
			result = format("width=%d,height=%d,rotation=%d", width, height, rotation);
		}
	}
	else if (strcasecmp(infoClass, "DeviceId") == 0)
	{
		char szDeviceId[PROP_NAME_MAX];
		__system_property_get("ro.serialno", szDeviceId);
		result = szDeviceId;
	}
	else if (strcasecmp(infoClass, "UserId") == 0)
	{
		result = format("%d", getuid());
	}
	else
	{
		iRet = ERR_INVALID_PARAMETER;
	}
	return iRet;
}
