#ifndef _COMMON_H
#define _COMMON_H

#include <stdint.h>
#include "buffer.h"
#include "strutil.h"
#include "fileutil.h"
#include "errcode.h"

#include <android/log.h>
#define DbgPrint(...)  __android_log_print(ANDROID_LOG_INFO, "asf-native", __VA_ARGS__)

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "asf-native", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "asf-native", __VA_ARGS__)

std::string GetModulePath(pid_t pid, const void *module_addr);
const void *GetModuleAddress(pid_t pid, const char *module_name);
uint8_t *MemorySearch(uint8_t *baseAddr, size_t regionSize, const char *featureCodes);
uint32_t GetTickCount();

#endif
