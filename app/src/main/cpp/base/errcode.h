//
// Created by Jack on 2019/3/3.
//

#ifndef ASF_ERRCODE_H
#define ASF_ERRCODE_H

#define IS_SUCCESS(r) ((int)r >= 0)

#define ERR_NONE 0
#define ERR_INVALID_INVOKE -1
#define ERR_INVALID_PARAMETER -2
#define ERR_INVOKE_FAILED -3
#define ERR_OPEN_FILE_FAILED -4
#define ERR_SAVE_FILE_FAILED -5
#define ERR_WINAPI_INVOKE_FAILED -6
#define ERR_NOT_FOUND -7
#define ERR_NOT_EXIST -8
#define ERR_RUN_APP_FAILED -9
#define ERR_INVALID_ACCOUNT -10
#define ERR_ALLOC_MEMORY_FAILED -11
#define ERR_BKAPI_INVOKE_FALIED -12
#define ERR_SLAVE_DISCONNECTED -13
#define ERR_NAME_NOT_EXIST -14
#define ERR_BKAPI_RETURN_ERROR -15
#define ERR_AREA_INVALID -16
#define ERR_WINDOW_BLOCKED -17
#define ERR_WINDOW_BOUND -18
#define ERR_LOAD_LOADER_FAILED -19
#define ERR_INVALID_WINDOW -20
#define ERR_LOAD_BKAPI_FAILED -21
#define ERR_INIT_CAPTURE_FAILED -22
#define ERR_INIT_BKGND_FAILED -23
#define ERR_INVALID_EXPRESSION -24
#define ERR_READ_MEMORY_FAILED -25
#define ERR_WRITE_MEMORY_FAILED -26
#define ERR_GET_MODULE_ADDR_FAILED -27
#define ERR_LOAD_DLL_EXTEND_FAILED -28
#define ERR_TIME_OUT -29
#define ERR_USER_ABORT -30
#define ERR_ACCESS_DENIED -31

#endif //ASF_ERRCODE_H
