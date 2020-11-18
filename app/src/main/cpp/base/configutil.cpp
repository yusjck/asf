//
// Created by Jack on 2019/7/22.
//

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "strutil.h"
#include "configutil.h"

ConfigUtil::ConfigUtil()
{
}

ConfigUtil::~ConfigUtil()
{
}

std::string ConfigUtil::GetString(const char *name)
{
	return GetString(name, "");
}

std::string ConfigUtil::GetString(const char *name, const char *defaultValue)
{
	std::string value;
	if (name)
	{
		auto iter = m_configs.find(name);
		if (iter == m_configs.end())
			value = defaultValue;		// 变量不存在则返回默认值
		else
			value = iter->second;
	}
	else
	{
		// name为NULL则返回所有变量名
		auto iter = m_configs.begin();
		if (iter == m_configs.end())
			value = defaultValue;
		else
		{
			do
			{
				if (value.empty())
					value = iter->first;
				else
					value += "\n" + iter->first;
				iter++;
			} while (iter != m_configs.end());
		}
	}
	return value;
}

void ConfigUtil::SetString(const char *name, const char *value)
{
	if (name)
	{
		if (value)
			m_configs[name] = value;
		else
		{
			// value为NULL就删除变量
			auto iter = m_configs.find(name);
			if (iter != m_configs.end())
				m_configs.erase(iter);
		}
	}
	else
	{
		// name为NULL则清空所有变量
		m_configs.clear();
	}
}

int ConfigUtil::GetInt(const char *name)
{
	return GetInt(name, 0);
}

int ConfigUtil::GetInt(const char *name, int defaultValue)
{
	char str[20];
	sprintf(str, "%d", defaultValue);
	std::string res = GetString(name, str);
	return atoi(res.c_str());
}

void ConfigUtil::SetInt(const char *name, int value)
{
	char str[20];
	sprintf(str, "%d", value);
	SetString(name, str);
}

bool ConfigUtil::GetBool(const char *name)
{
	return GetBool(name, false);
}

bool ConfigUtil::GetBool(const char *name, bool defaultValue)
{
	std::string res = GetString(name, defaultValue ? "1" : "0");
	return res == "1";
}

void ConfigUtil::SetBool(const char *name, bool value)
{
	SetString(name, value ? "1" : "0");
}

std::vector<std::string> ConfigUtil::GetNameList()
{
	std::vector<std::string> nameList;
	auto iter = m_configs.begin();
	while (iter != m_configs.end())
	{
		nameList.push_back(iter->first);
		iter++;
	}
	return nameList;
}

void ConfigUtil::InitConfigs(const char *configString)
{
	RemoveAll();
	UpdateConfigs(configString);
}

void ConfigUtil::UpdateConfigs(const char *configString)
{
	std::vector<std::string> rows = split(configString, "|");
	std::vector<std::string>::iterator iter = rows.begin();
	for (; iter != rows.end(); iter++)
	{
		std::vector<std::string> row = split(iter->c_str(), "=");
		if (row.size() != 2)
			continue;
		std::string key = row[0];
		std::string value = row[1];
		string_replace(value, "#2", "|");
		string_replace(value, "#3", "=");
		string_replace(value, "#1", "#");
		m_configs[key] = value;
	}
}

bool ConfigUtil::Remove(const char *name)
{
	bool result = false;
	auto iter = m_configs.find(name);
	if (iter != m_configs.end())
	{
		m_configs.erase(iter);
		result = true;
	}
	return result;
}

void ConfigUtil::RemoveAll()
{
	m_configs.clear();
}
