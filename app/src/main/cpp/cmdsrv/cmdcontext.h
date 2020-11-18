#ifndef CMDCONTEXT_H
#define CMDCONTEXT_H

#pragma warning(disable : 4200)

#include <iostream>
#include <string>
#include <vector>
#include <stdint.h>
#include "buffer.h"
#include "handle.h"

typedef enum {
	DT_VOID = 0,
	DT_NULL,
	DT_BOOL,
	DT_INT,
	DT_FLOAT,
	DT_STR,
	DT_HANDLE,
	DT_BIN,
	DT_UINT,
	DT_CMDCODE,
	DT_RETCODE,
} DATA_TYPE;

typedef struct {
	DATA_TYPE dt;
	uint32_t datalen;
	uint8_t data[0];
} PARAM_NODE;

class CCmdContext
{
public:
	int GetNodeCount();
	DATA_TYPE GetNodeType(int idx);
	int GetCmdCode();
	int GetRetCode();
	bool GetBool(int idx);
	int GetInt(int idx);
	uint32_t GetUint(int idx);
	int64_t GetInt64(int idx);
	float GetFloat(int idx);
	double GetDouble(int idx);
	const char *GetStr(int idx);
	const char *GetStr(int idx, uint32_t &len);
	handle_t GetHandle(int idx);
	const void *GetBin(int idx, uint32_t &len);

	__inline void Get(int idx, bool &v) { v = GetBool(idx); }
	__inline void Get(int idx, int &v) { v = GetInt(idx); }
	__inline void Get(int idx, uint32_t &v) { v = GetUint(idx); }
	__inline void Get(int idx, int64_t &v) { v = GetInt64(idx); }
	__inline void Get(int idx, float &v) { v = GetFloat(idx); }
	__inline void Get(int idx, double &v) { v = GetDouble(idx); }
	__inline void Get(int idx, const char *&s) { s = GetStr(idx); }
	__inline void Get(int idx, std::string &s) { s = GetStr(idx); }
	__inline void Get(int idx, handle_t &h) { h = GetHandle(idx); }

	void WriteBool(bool v);
	void WriteInt(int v);
	void WriteUint(uint32_t v);
	void WriteInt64(int64_t v);
	void WriteFloat(float v);
	void WriteDouble(double v);
	void WriteStr(const char *str);
	void WriteStr(const char *str, uint32_t len);
	void WriteHandle(handle_t h);
	void WriteBin(const void *bin, uint32_t len);

	__inline void Write(bool v) { WriteBool(v); }
	__inline void Write(int v) { WriteInt(v); }
	__inline void Write(uint32_t v) { WriteUint(v); }
	__inline void Write(int64_t v) { WriteInt64(v); }
	__inline void Write(float v) { WriteFloat(v); }
	__inline void Write(double v) { WriteDouble(v); }
	__inline void Write(const char *s) { WriteStr(s); }
	__inline void Write(std::string &s) { WriteStr(s.c_str()); }
	__inline void Write(handle_t h) { WriteHandle(h); }
	__inline void Write(const CBuffer &buf)
	{
		WriteBin(buf.GetBuffer(), (uint32_t)buf.GetSize());
	}

	void InitCmdContext(int cmdcode);
	void InitRetContext(int retcode);
	bool ParseCmdContext(CBuffer &ctx);
	bool ParseRetContext(CBuffer &ctx);

	CCmdContext();
	~CCmdContext();

public:
	CBuffer &GetCmdCtx() { return m_pCmdCtx; }
	CBuffer &GetRetCtx() { return m_pRetCtx; }

private:
	std::vector<PARAM_NODE *> m_args;
	CBuffer m_pCmdCtx;
	CBuffer m_pRetCtx;
	CBuffer *m_pWriteCtx;

	bool ParseContext(CBuffer &ctx);
	void CheckDataType(int idx, DATA_TYPE dt, int datalen = 0);
	void WriteData(DATA_TYPE dt, const void *data, size_t datalen);
	void WriteString(const char *str, size_t len);
	CCmdContext(const CCmdContext &);
	CCmdContext &operator=(const CCmdContext &);
};

#define PB_CREATE_CTX(cmd)\
	CCmdContext _ctx;\
	_ctx.InitCmdContext(cmd);

#define PB_WRITE_VALUES1(a1)\
							_ctx.Write(a1);
#define PB_WRITE_VALUES2(a1, a2)\
							PB_WRITE_VALUES1(a1)\
							_ctx.Write(a2);
#define PB_WRITE_VALUES3(a1, a2, a3)\
							PB_WRITE_VALUES2(a1, a2)\
							_ctx.Write(a3);
#define PB_WRITE_VALUES4(a1, a2, a3, a4)\
							PB_WRITE_VALUES3(a1, a2, a3)\
							_ctx.Write(a4);
#define PB_WRITE_VALUES5(a1, a2, a3, a4, a5)\
							PB_WRITE_VALUES4(a1, a2, a3, a4)\
							_ctx.Write(a5);
#define PB_WRITE_VALUES6(a1, a2, a3, a4, a5, a6)\
							PB_WRITE_VALUES5(a1, a2, a3, a4, a5)\
							_ctx.Write(a6);
#define PB_WRITE_VALUES7(a1, a2, a3, a4, a5, a6, a7)\
							PB_WRITE_VALUES6(a1, a2, a3, a4, a5, a6)\
							_ctx.Write(a7);
#define PB_WRITE_VALUES8(a1, a2, a3, a4, a5, a6, a7, a8)\
							PB_WRITE_VALUES7(a1, a2, a3, a4, a5, a6, a7)\
							_ctx.Write(a8);
#define PB_WRITE_VALUES9(a1, a2, a3, a4, a5, a6, a7, a8, a9)\
							PB_WRITE_VALUES8(a1, a2, a3, a4, a5, a6, a7, a8)\
							_ctx.Write(a9);
#define PB_WRITE_VALUES10(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10)\
							PB_WRITE_VALUES9(a1, a2, a3, a4, a5, a6, a7, a8, a9)\
							_ctx.Write(a10);
#define PB_GET_VALUES1(a1)\
							_ctx.Get(1, a1);
#define PB_GET_VALUES2(a1, a2)\
							PB_GET_VALUES1(a1)\
							_ctx.Get(2, a2);
#define PB_GET_VALUES3(a1, a2, a3)\
							PB_GET_VALUES2(a1, a2)\
							_ctx.Get(3, a3);
#define PB_GET_VALUES4(a1, a2, a3, a4)\
							PB_GET_VALUES3(a1, a2, a3)\
							_ctx.Get(4, a4);
#define PB_GET_VALUES5(a1, a2, a3, a4, a5)\
							PB_GET_VALUES4(a1, a2, a3, a4)\
							_ctx.Get(5, a5);

#define PB_GET_VALUES_TYPE1(t1)\
							t1 a1; _ctx.Get(1, a1);
#define PB_GET_VALUES_TYPE2(t1, t2)\
							PB_GET_VALUES_TYPE1(t1)\
							t2 a2; _ctx.Get(2, a2);
#define PB_GET_VALUES_TYPE3(t1, t2, t3)\
							PB_GET_VALUES_TYPE2(t1, t2)\
							t3 a3; _ctx.Get(3, a3);
#define PB_GET_VALUES_TYPE4(t1, t2, t3, t4)\
							PB_GET_VALUES_TYPE3(t1, t2, t3)\
							t4 a4; _ctx.Get(4, a4);
#define PB_GET_VALUES_TYPE5(t1, t2, t3, t4, t5)\
							PB_GET_VALUES_TYPE4(t1, t2, t3, t4)\
							t5 a5; _ctx.Get(5, a5);
#define PB_GET_VALUES_TYPE6(t1, t2, t3, t4, t5, t6)\
							PB_GET_VALUES_TYPE5(t1, t2, t3, t4, t5)\
							t6 a6; _ctx.Get(6, a6);
#define PB_GET_VALUES_TYPE7(t1, t2, t3, t4, t5, t6, t7)\
							PB_GET_VALUES_TYPE6(t1, t2, t3, t4, t5, t6)\
							t7 a7; _ctx.Get(7, a7);
#define PB_GET_VALUES_TYPE8(t1, t2, t3, t4, t5, t6, t7, t8)\
							PB_GET_VALUES_TYPE7(t1, t2, t3, t4, t5, t6, t7)\
							t8 a8; _ctx.Get(8, a8);
#define PB_GET_VALUES_TYPE9(t1, t2, t3, t4, t5, t6, t7, t8, t9)\
							PB_GET_VALUES_TYPE8(t1, t2, t3, t4, t5, t6, t7, t8)\
							t9 a9; _ctx.Get(9, a9);
#define PB_GET_VALUES_TYPE10(t1, t2, t3, t4, t5, t6, t7, t8, t9, t10)\
							PB_GET_VALUES_TYPE9(t1, t2, t3, t4, t5, t6, t7, t8, t9)\
							t10 a10; _ctx.Get(10, a10);

#endif
