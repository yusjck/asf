#ifndef PLUGIN_H
#define PLUGIN_H

#include <map>
#include "buffer.h"

class PluginManager
{
public:
	typedef enum {
		LOCAL_CMD,
		REMOTE_CMD,
		ALL_CMD
	} CMD_TYPE;
	void CleanupPlugins(const char *pluginDir, const char *reservedList);
	void LoadPlugins(const char *pluginDir, uint32_t scriptId, bool runInMicroServer);
	bool GetCommandList(std::vector<std::string> &cmdList, CMD_TYPE cmdType = ALL_CMD);
	bool IsRemoteCommand(const char *commandName);
	bool InvokePluginCommand(const char *commandName, const void *invokeData, uint32_t invokeDataSize, CBuffer &retval);
	void DestroyPluginObject(const char *commandName, void *object);
	static void SetDefaultPluginDir(const char *pluginDir) { m_defaultPluginDir = pluginDir; }
	static std::string GetDefaultPluginDir() { return m_defaultPluginDir; }
	PluginManager();
	virtual ~PluginManager();

private:
	typedef struct _PLUGIN_COMMAND_DESC {
		CMD_TYPE CommandType;
	} PLUGIN_COMMAND_DESC, *PPLUGIN_COMMAND_DESC;

	typedef std::map<std::string, PLUGIN_COMMAND_DESC> PLUGIN_COMMAND_LIST;

	typedef struct _PLUGIN_MODULE_INFO {
		std::string ModuleFileName;
		void *PluginModuleInstance;
		void *PluginClassInstance;
		PLUGIN_COMMAND_LIST CommandList;
		void *funInvokeBegin;
		void *funInvokeEnd;
		void *funDestroyObject;
	} PLUGIN_MODULE_INFO, *PPLUGIN_MODULE_INFO;

	typedef std::map<std::string, PLUGIN_MODULE_INFO> PLUGIN_MODULE_LIST;

	static std::string m_defaultPluginDir;
	PLUGIN_MODULE_LIST m_pluginList;
	bool m_pluginLoaded;
	uint32_t m_scriptId;
	bool m_runInMicroServer;

	bool LoadPlugin(const char *pluginDir, const char *pluginName);
	void UnloadPlugins();
	bool FindCommandModule(const char *commandName, PLUGIN_MODULE_INFO &pluginModuleInfo);
	bool FindCommandDesc(const char *commandName, PLUGIN_COMMAND_DESC &pluginCommandDesc);
};

#endif