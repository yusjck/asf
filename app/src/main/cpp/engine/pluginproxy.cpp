#include "pluginproxy.h"

PluginProxy::PluginProxy()
= default;

PluginProxy::~PluginProxy()
= default;

int PluginProxy::LoadPlugins(const char *pluginDir, uint32_t scriptId, bool runInMicroServer)
{
	// 本地加载插件
	m_pluginManager.LoadPlugins(pluginDir, scriptId, runInMicroServer);
	return ERR_NONE;
}

bool PluginProxy::GetCommandList(std::vector<std::string> &cmdList)
{
	return m_pluginManager.GetCommandList(cmdList);
}

int PluginProxy::InvokePluginCommand(const char *commandName, const void *invokeData, uint32_t invokeDataSize, CBuffer &retval)
{
	return m_pluginManager.InvokePluginCommand(commandName, invokeData, invokeDataSize, retval);
}

int PluginProxy::DestroyPluginObject(const char *commandName, void *object)
{
	m_pluginManager.DestroyPluginObject(commandName, object);
	return ERR_NONE;
}
