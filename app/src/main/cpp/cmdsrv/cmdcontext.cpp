#include <assert.h>
#include "cmdcontext.h"

#define max(a,b)    (((a) > (b)) ? (a) : (b))
#define min(a,b)    (((a) < (b)) ? (a) : (b))

CCmdContext::CCmdContext()
{
	m_pWriteCtx = NULL;
}

CCmdContext::~CCmdContext()
{
}

void CCmdContext::InitCmdContext(int cmdcode)
{
	if (m_pCmdCtx.IsEmpty())
	{
		m_pWriteCtx = &m_pCmdCtx;
		WriteData(DT_CMDCODE, &cmdcode, sizeof(cmdcode));
	}
}

void CCmdContext::InitRetContext(int retcode)
{
	if (m_pRetCtx.IsEmpty())
	{
		m_pWriteCtx = &m_pRetCtx;
		WriteData(DT_RETCODE, &retcode, sizeof(retcode));
	}
}

bool CCmdContext::ParseCmdContext(CBuffer &ctx)
{
	if (m_pCmdCtx.IsEmpty())
	{
		m_pCmdCtx = ctx;
		return ParseContext(ctx);
	}
	return false;
}

bool CCmdContext::ParseRetContext(CBuffer &ctx)
{
	if (m_pRetCtx.IsEmpty())
	{
		m_pRetCtx = ctx;
		return ParseContext(ctx);
	}
	return false;
}

bool CCmdContext::ParseContext(CBuffer &ctx)
{
	m_args.clear();

	size_t param_len = ctx.GetSize();
	uint8_t *param_ptr = (uint8_t *)ctx.GetBuffer();
	size_t read_len = 0;
	while (read_len < param_len)
	{
		PARAM_NODE *p = (PARAM_NODE *)(param_ptr + read_len);
		if (p->dt == DT_VOID)
			break;
		switch (p->dt)
		{
			case DT_BOOL:
			case DT_NULL:
			case DT_INT:
			case DT_FLOAT:
			case DT_STR:
			case DT_HANDLE:
			case DT_BIN:
			case DT_UINT:
			case DT_CMDCODE:
			case DT_RETCODE:
				break;
			default:
				return false;
		}
		m_args.push_back(p);
		size_t alignlen = (p->datalen + (p->dt == DT_STR) + 7) & ~7L;
		read_len += sizeof(PARAM_NODE) + alignlen;
		if (read_len > param_len)
			return false;
	}
	return m_args.size() > 0;
}

void CCmdContext::CheckDataType(int idx, DATA_TYPE dt, int datalen)
{
	if (idx >= (int)m_args.size())
		throw "Invalid parameter index";
	if (m_args[idx]->dt != dt)
		throw "Parameter type error";
	if (datalen > 0 && m_args[idx]->datalen != datalen)
		throw "Parameter length error";
}

int CCmdContext::GetNodeCount()
{
	return m_args.size();
}

DATA_TYPE CCmdContext::GetNodeType(int idx)
{
	if (idx >= (int)m_args.size())
		throw "Invalid parameter index";
	return m_args[idx]->dt;
}

int CCmdContext::GetCmdCode()
{
	CheckDataType(0, DT_CMDCODE);
	return *(int *)m_args[0]->data;
}

int CCmdContext::GetRetCode()
{
	CheckDataType(0, DT_RETCODE);
	return *(int *)m_args[0]->data;
}

bool CCmdContext::GetBool(int idx)
{
	CheckDataType(idx, DT_BOOL);
	return *(bool *)m_args[idx]->data;
}

int CCmdContext::GetInt(int idx)
{
	CheckDataType(idx, DT_INT);
	return *(int *)m_args[idx]->data;
}

uint32_t CCmdContext::GetUint(int idx)
{
	CheckDataType(idx, DT_UINT);
	return *(uint32_t *)m_args[idx]->data;
}

int64_t CCmdContext::GetInt64(int idx)
{
	CheckDataType(idx, DT_INT, sizeof(int64_t));
	return *(int64_t *)m_args[idx]->data;
}

float CCmdContext::GetFloat(int idx)
{
	CheckDataType(idx, DT_FLOAT, sizeof(float));
	return *(float *)m_args[idx]->data;
}

double CCmdContext::GetDouble(int idx)
{
	CheckDataType(idx, DT_FLOAT, sizeof(double));
	return *(double *)m_args[idx]->data;
}

const char *CCmdContext::GetStr(int idx)
{
	if (GetNodeType(idx) == DT_NULL)
		return NULL;
	CheckDataType(idx, DT_STR);
	return (const char *)m_args[idx]->data;
}

const char *CCmdContext::GetStr(int idx, uint32_t &len)
{
	if (GetNodeType(idx) == DT_NULL)
		return NULL;
	CheckDataType(idx, DT_STR);
	len = m_args[idx]->datalen;
	return (const char *)m_args[idx]->data;
}

handle_t CCmdContext::GetHandle(int idx)
{
	CheckDataType(idx, DT_HANDLE, sizeof(uint32_t));
	handle_t handle = (handle_t)((uintptr_t)*(uint32_t *)m_args[idx]->data);
	return handle;
}

const void *CCmdContext::GetBin(int idx, uint32_t &len)
{
	CheckDataType(idx, DT_BIN);
	len = m_args[idx]->datalen;
	return m_args[idx]->data;
}

void CCmdContext::WriteBool(bool v)
{
	WriteData(DT_BOOL, &v, sizeof(v));
}

void CCmdContext::WriteInt(int v)
{
	WriteData(DT_INT, &v, sizeof(v));
}

void CCmdContext::WriteUint(uint32_t v)
{
	WriteData(DT_UINT, &v, sizeof(v));
}

void CCmdContext::WriteInt64(int64_t v)
{
	WriteData(DT_INT, &v, sizeof(v));
}

void CCmdContext::WriteFloat(float v)
{
	WriteData(DT_FLOAT, &v, sizeof(v));
}

void CCmdContext::WriteDouble(double v)
{
	WriteData(DT_FLOAT, &v, sizeof(v));
}

void CCmdContext::WriteStr(const char *str)
{
	WriteString(str, strlen(str));
}

void CCmdContext::WriteStr(const char *str, uint32_t len)
{
	WriteString(str, len);
}

void CCmdContext::WriteHandle(handle_t h)
{
	WriteData(DT_HANDLE, &h, sizeof(uint32_t));
}

void CCmdContext::WriteBin(const void *bin, uint32_t len)
{
	WriteData(DT_BIN, bin, len);
}

void CCmdContext::WriteData(DATA_TYPE dt, const void *data, size_t datalen)
{
	size_t alignlen = (datalen + (dt == DT_STR) + 7) & ~7L;
	PARAM_NODE *p = (PARAM_NODE *)m_pWriteCtx->GetBufferEnd(sizeof(PARAM_NODE) + alignlen);
	if (p == NULL)
		throw "no memory";
	p->dt = dt;
	p->datalen = datalen;
	memcpy(p->data, data, datalen);
	memset(p->data + datalen, 0, alignlen - datalen);
}

void CCmdContext::WriteString(const char *str, size_t len)
{
	if (str == NULL)
	{
		WriteData(DT_NULL, NULL, 0);
		return;
	}
	if (len == -1)
		len = strlen(str);
	WriteData(DT_STR, str, len);
}
