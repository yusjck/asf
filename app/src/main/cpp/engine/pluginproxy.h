//
// Created by Jack on 2019/6/9.
//

#ifndef ASF_PLUGINPROXY_H
#define ASF_PLUGINPROXY_H

#include "common.h"
#include "../control/pluginmgr.h"

enum
{
	IVT_NIL,
	IVT_BOOLEAN,
	IVT_NUMBER,
	IVT_STRING,
	IVT_POINTER,
	IVT_BINARY,
	IVT_OBJECT,
};

typedef struct _DATA_HEAD {
	int type;
	union {
		uint32_t data2len;
		uint8_t data1[1];
	};
	uint8_t data2[1];
} DATA_HEAD, *PDATA_HEAD;

class PluginDataWriter
{
public:
	void PushNil()
	{
		WriteData(IVT_NIL, NULL, 0);
	}

	void PushBoolean(int val)
	{
		WriteData(IVT_BOOLEAN, &val, sizeof(val));
	}

	void PushNumber(double val)
	{
		WriteData(IVT_NUMBER, &val, sizeof(val));
	}

	void PushString(const char *str, uint32_t len)
	{
		WriteData(IVT_STRING, str, len);
	}

	void PushPointer(const void *ptr)
	{
		WriteData(IVT_POINTER, &ptr, sizeof(ptr));
	}

	void PushBinary(const void *data, uint32_t datalen)
	{
		WriteData(IVT_BINARY, data, datalen);
	}

	void PushObject(void *obj)
	{
		WriteData(IVT_OBJECT, &obj, sizeof(obj));
	}

	const CBuffer &GetBuf()
	{
		return m_buf;
	}

	PluginDataWriter()
	{

	}

	~PluginDataWriter()
	{

	}

private:
	CBuffer m_buf;

	void WriteData(int type, const void *data, size_t datalen)
	{
		if (type == IVT_NIL || type == IVT_BOOLEAN || type == IVT_NUMBER || type == IVT_POINTER || type == IVT_OBJECT)
		{
			PDATA_HEAD p = (PDATA_HEAD) m_buf.GetBufferEnd(offsetof(DATA_HEAD, data1) + datalen);
			p->type = type;
			memcpy(p->data1, data, datalen);
		}
		else if (type == IVT_STRING || type == IVT_BINARY)
		{
			size_t alignlen = (datalen + (type == IVT_STRING) + 3) & ~3;
			PDATA_HEAD p = (PDATA_HEAD)m_buf.GetBufferEnd(offsetof(DATA_HEAD, data2) + alignlen);
			p->type = type;
			p->data2len = datalen;
			memcpy(p->data2, data, datalen);
			memset(&p->data2[datalen], 0, alignlen - datalen);
		}
	}
};

class PluginDataReader
{
public:
	bool ParseDataBuf(const CBuffer &buf)
	{
		m_valEntry.clear();
		m_buf = buf;
		size_t n = 0;

		for (;;)
		{
			if (n == buf.GetSize())
				return true;

			if (n + offsetof(DATA_HEAD, data1) > buf.GetSize())
				return false;

			PDATA_HEAD p = (PDATA_HEAD)((uint8_t *)buf.GetBuffer() + n);
			switch (p->type)
			{
				case IVT_NIL:
					m_valEntry.push_back(p);
					n += offsetof(DATA_HEAD, data1);
					break;
				case IVT_BOOLEAN:
					m_valEntry.push_back(p);
					n += offsetof(DATA_HEAD, data1);
					n += sizeof(int);
					break;
				case IVT_NUMBER:
					m_valEntry.push_back(p);
					n += offsetof(DATA_HEAD, data1);
					n += sizeof(double);
					break;
				case IVT_STRING:
					m_valEntry.push_back(p);
					n += offsetof(DATA_HEAD, data2);
					n += (p->data2len + 1 + 3) & ~3;
					break;
				case IVT_POINTER:
					m_valEntry.push_back(p);
					n += offsetof(DATA_HEAD, data1);
					n += sizeof(const void *);
					break;
				case IVT_BINARY:
					m_valEntry.push_back(p);
					n += offsetof(DATA_HEAD, data2);
					n += (p->data2len + 3) & ~3;
					break;
				case IVT_OBJECT:
					m_valEntry.push_back(p);
					n += offsetof(DATA_HEAD, data1);
					n += sizeof(void *);
					break;
				default:
					return false;
			}
		}
	}

	int GetValueType(int idx)
	{
		assert(idx < (int)m_valEntry.size());
		return m_valEntry[idx]->type;
	}

	int GetValueCount()
	{
		return m_valEntry.size();
	}

	int GetBoolean(int idx)
	{
		PDATA_HEAD p = m_valEntry[idx];
		assert(p->type == IVT_BOOLEAN);
		return *(int *)p->data1;
	}

	double GetNumber(int idx)
	{
		PDATA_HEAD p = m_valEntry[idx];
		assert(p->type == IVT_NUMBER);
		return *(double *)p->data1;
	}

	const char *GetString(int idx, uint32_t &len)
	{
		PDATA_HEAD p = m_valEntry[idx];
		assert(p->type == IVT_STRING);
		len = p->data2len;
		return (const char *)p->data2;
	}

	const void *GetPointer(int idx)
	{
		PDATA_HEAD p = m_valEntry[idx];
		assert(p->type == IVT_POINTER);
		return *(const void **)p->data1;
	}

	const void *GetBinary(int idx, uint32_t &len)
	{
		PDATA_HEAD p = m_valEntry[idx];
		assert(p->type == IVT_STRING);
		len = p->data2len;
		return (const void *)p->data2;
	}

	void *GetObject(int idx)
	{
		PDATA_HEAD p = m_valEntry[idx];
		assert(p->type == IVT_OBJECT);
		return *(void **)p->data1;
	}

	PluginDataReader()
	{

	}

	~PluginDataReader()
	{

	}

private:
	CBuffer m_buf;
	std::vector<PDATA_HEAD> m_valEntry;
};

class PluginProxy
{
public:
	int LoadPlugins(const char *pluginDir, uint32_t scriptId, bool runInMicroServer);
	bool GetCommandList(std::vector<std::string> &cmdList);
	int InvokePluginCommand(const char *commandName, const void *invokeData, uint32_t invokeDataSize, CBuffer &retval);
	int DestroyPluginObject(const char *commandName, void *object);

	PluginProxy();
	~PluginProxy();

private:
	PluginManager m_pluginManager;
};

#endif //ASF_PLUGINPROXY_H
