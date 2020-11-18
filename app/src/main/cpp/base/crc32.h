#ifndef _CRC32_H
#define _CRC32_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint32_t do_crc32(const void *buf, size_t len);

#ifdef __cplusplus
}
#endif

#endif
