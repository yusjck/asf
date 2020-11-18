#include "common.h"
#include <dirent.h>
#include <dlfcn.h>
#include <unistd.h>
#include "pluginmgr.h"

#define PLUGIN_COMPATIBLE_VERSION 19

typedef struct _PLUGIN_DESC {
	uint32_t PluginDescSize;
	uint32_t CompatibleVersion;
	uint32_t PluginVersion;
	const char *PluginName;
	const char *PluginDescription;
	const char *PluginCommands;
} PLUGIN_DESC, *PPLUGIN_DESC;

typedef struct _PROTO_BUF {
	const void *Buffer;
	uint32_t Length;
} PROTO_BUF, *PPROTOBUF;

typedef struct _INVOKE_DATA {
	const char *CommandName;
	PROTO_BUF Param;
	PROTO_BUF ReturnVal;
} INVOKE_DATA, *PINVOKE_DATA;

typedef int (*GET_PLUGIN_DESC)(PPLUGIN_DESC pluginDesc);
typedef void *(*PLUGIN_INIT)(uint32_t scriptId, int runInMicroServer);
typedef void (*PLUGIN_DESTROY)(void *pluginInstance);
typedef int (*INVOKE_BEGIN)(void *pluginInstance, PINVOKE_DATA invokeData);
typedef void (*INVOKE_END)(void *pluginInstance, PINVOKE_DATA invokeData);
typedef void (*DESTROY_OBJECT)(void *pluginInstance, void *object);

std::string PluginManager::m_defaultPluginDir;

PluginManager::PluginManager()
{
	m_pluginLoaded = false;
	m_scriptId = 0;
	m_runInMicroServer = false;
}

PluginManager::~PluginManager()
{
	UnloadPlugins();
}

void PluginManager::CleanupPlugins(const char *pluginDir, const char *reservedList)
{
	if (pluginDir == NULL)
		pluginDir = m_defaultPluginDir.c_str();

	DIR *ptr_dir = opendir(pluginDir);
	if (ptr_dir)
	{
		std::vector<std::string> pluginFilter = split(reservedList, "|");
		struct dirent *dir_entry;
		while ((dir_entry = readdir(ptr_dir)) != NULL)
		{
			if (!(dir_entry->d_type & DT_REG))
				continue;
			bool isReservedPlugin = false;
			for (size_t i = 0; i < pluginFilter.size(); i++)
			{
				if (strcmp(pluginFilter[i].c_str(), dir_entry->d_name) == 0)
				{
					isReservedPlugin = true;
					break;
				}
			}
			if (!isReservedPlugin)
			{
				char pluginPath[MAX_PATH];
				sprintf(pluginPath, "%s/%s", pluginDir, dir_entry->d_name);
				remove(pluginPath);
			}
		}
		closedir(ptr_dir);
	}
}

bool PluginManager::LoadPlugin(const char *pluginDir, const char *pluginName)
{
	char pluginPath[MAX_PATH];
	sprintf(pluginPath, "%s/%s", pluginDir, pluginName);

	void *moduleInst = dlopen(pluginPath, RTLD_LAZY);
	if (moduleInst == NULL)
		return false;

	GET_PLUGIN_DESC GetPluginDesc = (GET_PLUGIN_DESC)dlsym(moduleInst, "GetPluginDesc");
	if (GetPluginDesc == NULL)
	{
		dlclose(moduleInst);
		return false;
	}

	PLUGIN_DESC pluginDesc;
	pluginDesc.PluginDescSize = sizeof(PLUGIN_DESC);
	try
	{
		if (!GetPluginDesc(&pluginDesc) || pluginDesc.CompatibleVersion != PLUGIN_COMPATIBLE_VERSION)
		{
			dlclose(moduleInst);
			LOGI("Plugin %s is not compatible\n", pluginName);
			return false;
		}
	}
	catch (...)
	{
		dlclose(moduleInst);
		LOGI("Loading plugin %s exception, Please remove the plugin and try again\n", pluginName);
		return false;
	}

	PLUGIN_MODULE_LIST::iterator iter = m_pluginList.find(pluginDesc.PluginName);
	if (iter != m_pluginList.end())
	{
		dlclose(moduleInst);
		LOGI("Unable to load plugin %s because the plugin name conflicts with the plugin %s\n", pluginName, iter->second.ModuleFileName.c_str());
		return false;
	}

	PLUGIN_INIT PluginInit = (PLUGIN_INIT)dlsym(moduleInst, "PluginInit");
	void *pluginInstance = NULL;
	if (PluginInit)
	{
		try
		{
			pluginInstance = PluginInit(m_scriptId, m_runInMicroServer);
			if (!pluginInstance)
			{
				dlclose(moduleInst);
				LOGI("Plugin %s initialization failed\n", pluginName);
				return false;
			}
		}
		catch (...)
		{
			dlclose(moduleInst);
			LOGI("Plugin %s initialization routine exception\n", pluginName);
			return false;
		}
	}

	PLUGIN_MODULE_INFO pluginModuleInfo;
	pluginModuleInfo.ModuleFileName = pluginName;
	pluginModuleInfo.PluginModuleInstance = moduleInst;
	pluginModuleInfo.PluginClassInstance = pluginInstance;

	std::vector<std::string> cmdList = split(pluginDesc.PluginCommands, "|");
	for (size_t i = 0; i < cmdList.size(); i++)
	{
		std::vector<std::string> cmdDesc = split(cmdList[i], ",");
		if (cmdDesc.size() != 2)
			continue;
		PLUGIN_COMMAND_DESC pluginCommandDesc;
		pluginCommandDesc.CommandType = atoi(cmdDesc[1].c_str()) == 0 ? LOCAL_CMD : REMOTE_CMD;
		pluginModuleInfo.CommandList.insert(std::make_pair(cmdDesc[0], pluginCommandDesc));
	}

	pluginModuleInfo.funInvokeBegin = dlsym(moduleInst, "InvokeBegin");
	pluginModuleInfo.funInvokeEnd = dlsym(moduleInst, "InvokeEnd");
	pluginModuleInfo.funDestroyObject = dlsym(moduleInst, "DestroyObject");
	m_pluginList.insert(std::make_pair(pluginDesc.PluginName, pluginModuleInfo));
	return true;
}

void PluginManager::LoadPlugins(const char *pluginDir, uint32_t scriptId, bool runInMicroServer)
{
	if (m_pluginLoaded)
		return;

	if (pluginDir == NULL)
		pluginDir = m_defaultPluginDir.c_str();

	m_scriptId = scriptId;
	m_runInMicroServer = runInMicroServer;
	m_pluginLoaded = true;

	DIR *ptr_dir = opendir(pluginDir);
	if (ptr_dir)
	{
		struct dirent *dir_entry;
		while ((dir_entry = readdir(ptr_dir)) != NULL)
		{
			if (dir_entry->d_type & DT_REG)
				LoadPlugin(pluginDir, dir_entry->d_name);
		}
		closedir(ptr_dir);
	}
}

void PluginManager::UnloadPlugins()
{
	PLUGIN_MODULE_LIST::iterator iter = m_pluginList.begin();
	while (iter != m_pluginList.end())
	{
		void *moduleInst = iter->second.PluginModuleInstance;
		PLUGIN_DESTROY PluginDestroy = (PLUGIN_DESTROY)dlsym(moduleInst, "PluginDestroy");
		if (PluginDestroy)
		{
			try
			{
				PluginDestroy(iter->second.PluginClassInstance);
			}
			catch (...)
			{
				LOGI("Plugin %s destroy routine exception\n", iter->second.ModuleFileName.c_str());
			}
		}
		dlclose(moduleInst);
		m_pluginList.erase(iter++);
	}
	m_pluginLoaded = false;
}

bool PluginManager::GetCommandList(std::vector<std::string> &cmdList, CMD_TYPE cmdType)
{
	PLUGIN_MODULE_LIST::iterator iter = m_pluginList.begin();
	while (iter != m_pluginList.end())
	{
		const char *pluginName = iter->first.c_str();
		PLUGIN_COMMAND_LIST &pluginCommandList = iter->second.CommandList;
		PLUGIN_COMMAND_LIST::iterator iter1 = pluginCommandList.begin();
		while (iter1 != pluginCommandList.end())
		{
			const char *commandName = iter1->first.c_str();
			CMD_TYPE commandType = iter1->second.CommandType;
			if (cmdType == ALL_CMD || commandType == cmdType)
				cmdList.push_back(format("%s.%s", pluginName, commandName));
			iter1++;
		}
		iter++;
	}
	return true;
}

bool PluginManager::FindCommandModule(const char *commandName, PLUGIN_MODULE_INFO &pluginModuleInfo)
{
	std::vector<std::string> v = split(commandName, ".");
	if (v.size() != 2)
		return false;

	const char *pPluginName = v[0].c_str();

	PLUGIN_MODULE_LIST::iterator iter = m_pluginList.find(pPluginName);
	if (iter == m_pluginList.end())
		return false;

	pluginModuleInfo = iter->second;
	return true;
}

bool PluginManager::FindCommandDesc(const char *commandName, PLUGIN_COMMAND_DESC &pluginCommandDesc)
{
	std::vector<std::string> v = split(commandName, ".");
	if (v.size() != 2)
		return false;

	const char *pluginName = v[0].c_str();
	commandName = v[1].c_str();

	PLUGIN_MODULE_LIST::iterator iter = m_pluginList.find(pluginName);
	if (iter == m_pluginList.end())
		return false;

	PLUGIN_COMMAND_LIST &commandList = iter->second.CommandList;
	PLUGIN_COMMAND_LIST::iterator iter1 = commandList.find(commandName);
	if (iter1 == commandList.end())
		return false;

	pluginCommandDesc = iter1->second;
	return true;
}

bool PluginManager::IsRemoteCommand(const char *commandName)
{
	PLUGIN_COMMAND_DESC commandDesc;
	if (!FindCommandDesc(commandName, commandDesc))
	{
		return false;
	}

	return commandDesc.CommandType == REMOTE_CMD;
}

bool PluginManager::InvokePluginCommand(const char *commandName, const void *invokeData, uint32_t invokeDataSize, CBuffer &retval)
{
	PLUGIN_MODULE_INFO pluginModule;
	if (!FindCommandModule(commandName, pluginModule))
	{
		return false;
	}

	try
	{
		INVOKE_DATA InvokeData = {0};
		InvokeData.CommandName = strchr(commandName, '.') + 1;
		InvokeData.Param.Buffer = invokeData;
		InvokeData.Param.Length = invokeDataSize;

		// 将调用数据交给插件处理
		INVOKE_BEGIN InvokeBegin = (INVOKE_BEGIN)pluginModule.funInvokeBegin;
		if (!InvokeBegin(pluginModule.PluginClassInstance, &InvokeData))
			return false;

		// 取得插件返回的数据
		retval.Write(InvokeData.ReturnVal.Buffer, InvokeData.ReturnVal.Length);

		// 让插件在调用结束时有机会释放资源
		INVOKE_END InvokeEnd = (INVOKE_END)pluginModule.funInvokeEnd;
		InvokeEnd(pluginModule.PluginClassInstance, &InvokeData);
		return true;
	}
	catch (...)
	{
		LOGI("Invoke the plugin command exception, the command name: %s\n", commandName);
	}
	return false;
}

void PluginManager::DestroyPluginObject(const char *commandName, void *object)
{
	PLUGIN_MODULE_INFO pluginModule;
	if (!FindCommandModule(commandName, pluginModule))
	{
		return;
	}

	try
	{
		DESTROY_OBJECT DestroyObject = (DESTROY_OBJECT)pluginModule.funDestroyObject;
		DestroyObject(pluginModule.PluginClassInstance, object);
	}
	catch (...)
	{
		LOGI("Destroy the plugin object exception, the object source: %s\n", commandName);
	}
}
