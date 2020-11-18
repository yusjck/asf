//
// Created by Jack on 2019/7/22.
//

#ifndef ASF_CONFIGUTIL_H
#define ASF_CONFIGUTIL_H


#include <string>
#include <map>
#include <vector>

class ConfigUtil
{
public:
	std::string GetString(const char *name);
	std::string GetString(const char *name, const char *defaultValue);
	void SetString(const char *name, const char *value);

	int GetInt(const char *name);
	int GetInt(const char *name, int defaultValue);
	void SetInt(const char *name, int value);

	bool GetBool(const char *name);
	bool GetBool(const char *name, bool defaultValue);
	void SetBool(const char *name, bool value);

	std::vector<std::string> GetNameList();
	void InitConfigs(const char *configString);
	void UpdateConfigs(const char *configString);
	bool Remove(const char *name);
	void RemoveAll();
	ConfigUtil();
	~ConfigUtil();

private:
	std::map<const std::string, std::string> m_configs;
};


#endif //ASF_CONFIGUTIL_H
