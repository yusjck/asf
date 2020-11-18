#ifndef INJECT_H_
#define INJECT_H_

#include <sys/types.h>

int find_pid_of(const char *process_name);
int inject_remote_process(pid_t target_pid, const char *library_path, const char *function_name, const char *param);


#endif /* INJECT_H_ */
