#ifndef LOGFILE_H
#define LOGFILE_H

#include <time.h>

class LogFile
{
public:
	LogFile(void);
	~LogFile(void);

	bool Create(time_t logTime, const char *logPath);
	bool Create(const char *logPath);

	bool IsCreated();
	void Close();
	bool Write(time_t logTime, const char *log);
	bool Write(const char *log);

private:
	FILE *m_LogFile;
};

#endif
