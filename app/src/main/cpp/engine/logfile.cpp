#include "common.h"
#include <stdio.h>
#include <string>
#include <dirent.h>
#include "autolock.h"
#include "logfile.h"

CAutoLock g_CreateLogLock;

LogFile::LogFile(void)
{
	m_LogFile = NULL;
}


LogFile::~LogFile(void)
{
	Close();
}

bool LogFile::Create(time_t logTime, const char *logPath)
{
	if (IsCreated())
		return false;

	struct tm tm;
	localtime_r(&logTime, &tm);
	char szLogPrefix[20];
	strftime(szLogPrefix, sizeof(szLogPrefix), "%Y%m%d", &tm);
	int iTodayFileCount = 0;

	CAutoLock lock = g_CreateLogLock.GetLock();
	DIR *dir = opendir(logPath);
	if (dir)
	{
		for (;;)
		{
			struct dirent *file = readdir(dir);
			if (file == NULL)
				break;

			if (file->d_type == DT_DIR)        // 跳过目录
				continue;

			char *extname = strrchr(file->d_name, '.');
			if (extname == NULL || strcmp(extname, ".log") != 0)    // 过滤出目录下所有log文件
				continue;

			if (!strncmp(file->d_name, szLogPrefix, strlen(szLogPrefix)))    // 查找是否存在当天的日志文件
			{
				*extname = '\0';
				int iCount = atoi(file->d_name + 8);        // 获取日期后面的4位数字
				if (iCount >= iTodayFileCount)
					iTodayFileCount = iCount + 1;        // 如果存在当天的日志文件就增加iMaxCount，避免文件名重复
			}
		}
		closedir(dir);
	}

	char szLogPath[200];
	snprintf(szLogPath, sizeof(szLogPath), "%s/%s%04d.log", logPath, szLogPrefix, iTodayFileCount);
	m_LogFile = fopen(szLogPath, "wb");
	if (m_LogFile == NULL)
		return false;

	fseek(m_LogFile, 0, SEEK_END);
	return true;
}

bool LogFile::IsCreated()
{
	return m_LogFile != NULL;
}

void LogFile::Close()
{
	if (m_LogFile)
	{
		fclose(m_LogFile);
		m_LogFile = NULL;
	}
}

bool LogFile::Write(time_t logTime, const char *log)
{
	if (!IsCreated())
		return false;

	struct tm tm;
	localtime_r(&logTime, &tm);
	char szTime[64];
	strftime(szTime, sizeof(szTime), "[%m/%d %H:%M:%S] ", &tm);

	fwrite(szTime, strlen(szTime), 1, m_LogFile);
	fwrite(log, strlen(log), 1, m_LogFile);
	fwrite("\n", 1, 1, m_LogFile);
	fflush(m_LogFile);
	return true;
}

bool LogFile::Write(const char *log)
{
	return Write(time(NULL), log);
}
