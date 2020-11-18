#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include "common.h"

std::string GetModulePath(pid_t pid, const void *module_addr)
{
	std::string result;
	char filename[32];
	if (pid < 0)
		snprintf(filename, sizeof(filename), "/proc/self/maps");
	else
		snprintf(filename, sizeof(filename), "/proc/%d/maps", pid);

	FILE *fp = fopen(filename, "r");
	if (fp != NULL)
	{
		char line[1024];
		while (fgets(line, sizeof(line), fp))
		{
			const void *addr1, *addr2;
			char name[260] = {0};
			sscanf(line, "%p-%p %*s %*s %*s %*[^\x20]%s", &addr1, &addr2, name);
			if (module_addr >= addr1 && module_addr < addr2)
			{
				result = name;
				break;
			}
		}

		fclose(fp);
	}

	return result;
}

const void *GetModuleAddress(pid_t pid, const char *module_name)
{
	const void *result = 0;
	char filename[32];
	if (pid < 0)
		snprintf(filename, sizeof(filename), "/proc/self/maps");
	else
		snprintf(filename, sizeof(filename), "/proc/%d/maps", pid);

	FILE *fp = fopen(filename, "r");
	if (fp != NULL)
	{
		char line[1024];
		while (fgets(line, sizeof(line), fp))
		{
			const void *addr1, *addr2;
			char name[260] = {0};
			sscanf(line, "%p-%p %*s %*s %*s %*[^\x20]%s", &addr1, &addr2, name);
			char *p = strrchr(name, '/');
			if (p && strcmp(p + 1, module_name) == 0)
			{
				result = addr1;
				break;
			}
		}

		fclose(fp);
	}

	return result;
}

uint8_t *MemorySearch(uint8_t *baseAddr, size_t regionSize, const char *featureCodes)
{
	uint8_t ignore[100] = {0};
	uint8_t code[100];
	size_t nCodeSize = strlen(featureCodes) / 2;
	size_t iCodeStart = 0;
	bool bHeadIgnore = false;
	size_t iMemOffset, iMatchCount = 0;

	// 内存长度小于要搜索的特征码长度？
	if (regionSize < nCodeSize)
		return NULL;

	// 如果特征码大于200十六进制字符即100个字节或长度不是偶数则返回错误
	if (strlen(featureCodes) > 200 || (strlen(featureCodes) & 1))
		return NULL;

	for (size_t i = 0; i < nCodeSize; i++)
	{
		if (featureCodes[i * 2] == '?' || featureCodes[i * 2 + 1] == '?')
		{
			if (i == 0)
				bHeadIgnore = true;
			if (bHeadIgnore)
				iCodeStart++;		// iCodeStart记录特征码开头的??数量
			ignore[i] = 1;
		}
		else
		{
			// 将转换十六进制字符串为二进制数组以便进行比较
			uint32_t hexCode;
			if (sscanf(&(featureCodes[i * 2]), "%2x", &hexCode) == 0)
				return NULL;
			code[i] = (uint8_t)hexCode;
			bHeadIgnore = false;
		}
	}

	iMemOffset = 0;

	for (size_t i = iCodeStart; i < regionSize; i++)
	{
		// 检查该处的特征码是否是??，是的话直接跳过
		if (ignore[iCodeStart + iMatchCount])
		{
			// 已完成全部特征码匹配？
			if (iCodeStart + ++iMatchCount >= nCodeSize)
				break;
		}
			// 比较当前特征码是否匹配
		else if (baseAddr[i] == code[iCodeStart + iMatchCount])
		{
			// 已完成全部特征码匹配？
			if (iCodeStart + ++iMatchCount >= nCodeSize)
				break;
		}
		else
		{
			// 匹配失败，返回上一个位置重新搜索整个特征码
			iMatchCount = 0;		// 清除已匹配的字节数
			// 将搜索起点回滚到上一次搜索子串的位置
			i = iMemOffset++ + iCodeStart;
		}
	}

	// 检查匹配长度是否等于特征码长度，是就说明子串已搜索到
	if (iCodeStart + iMatchCount < nCodeSize)
	{
		return NULL;
	}

	return baseAddr + iMemOffset;
}

uint32_t GetTickCount()
{
	struct timespec time = {0};
	clock_gettime(CLOCK_MONOTONIC, &time);
	return (uint32_t)(time.tv_sec * 1000 + time.tv_nsec / 1000000);
}
