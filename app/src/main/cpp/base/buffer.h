#ifndef _BUFFER_H
#define _BUFFER_H

#include <stdlib.h>
#include <stdint.h>
#include <memory.h>
#include <assert.h>

#ifdef _MSC_VER
#pragma warning(disable: 4200)
#endif

#define MEM_ALLOC_RESERVE_SIZE 64

#define max(a,b)    (((a) > (b)) ? (a) : (b))
#define min(a,b)    (((a) < (b)) ? (a) : (b))

class CBuffer
{
private:
	typedef struct _BUFFER {
		uint32_t RefCount;
		size_t AllocLength;
		size_t Length;
		uint8_t *Buffer;
	} BUFFER, *PBUFFER;

public:
	// 返回指针总是指向缓冲区头部，无论之前是否写入过数据
	void *GetBuffer(size_t cbSize)
	{
		if (m_pBuffer == NULL)
			CreateBuffer();

		// 如果缓冲区已被其它对象引用且再重新分配一块缓冲区
		if (m_pBuffer->RefCount > 1)
		{
			PBUFFER pBuffer = CopyBuffer(cbSize);
			if (pBuffer == NULL)
				return NULL;
			DestoryBuffer();
			m_pBuffer = pBuffer;
		}

		if (cbSize <= m_pBuffer->AllocLength)
		{
			m_pBuffer->Length = cbSize;
			return m_pBuffer->Buffer;
		}

		uint8_t *pNewBuffer = (uint8_t *)realloc(m_pBuffer->Buffer, cbSize + MEM_ALLOC_RESERVE_SIZE);
		if (pNewBuffer == NULL)
			return NULL;

		m_pBuffer->Buffer = pNewBuffer;
		m_pBuffer->Length = cbSize;
		m_pBuffer->AllocLength = cbSize + MEM_ALLOC_RESERVE_SIZE;
		return m_pBuffer->Buffer;
	}

	// 为缓冲区指定一个新的大小
	void ReleaseBuffer(size_t cbNewSize = 0)
	{
		if (m_pBuffer)
		{
			if (cbNewSize < m_pBuffer->Length)
				GetBuffer(cbNewSize);
		}
	}

	// 释放掉已分配的缓冲区
	void FreeBuffer()
	{
		DestoryBuffer();
	}

	// 从缓冲区末尾分配一块新的空间并返回空间的首地址
	void *GetBufferEnd(size_t cbAppendSize)
	{
		size_t n = GetSize();
		void *p = GetBuffer(n + cbAppendSize);
		if (p)
			return (char *)p + n;
		return NULL;
	}

	// 指定缓冲区已使用的大小，然后保留一块未使用的内存，并将写入指针指向这块内存
	bool SetReserveSize(size_t cbUsedSize, size_t cbUnusedSize = 0)
	{
		if (!GetBuffer(cbUsedSize + cbUnusedSize))
			return false;
		ReleaseBuffer(cbUsedSize);
		return true;
	}

	// 如果当前缓冲区内已有数据将从数据末尾进行追加
	bool Write(const void *pData, size_t cbSize)
	{
		size_t cbOldSize = GetSize();
		void *p = GetBuffer(cbOldSize + cbSize);
		if (p == NULL)
			return false;
		memcpy((uint8_t *)p + cbOldSize, pData, cbSize);
		return true;
	}

	// nIndex为负数时表示是从缓冲区末尾往前计算的
	bool Insert(int nIndex, const void *pData, size_t cbSize)
	{
		size_t cbOldSize = GetSize();
		size_t iWritePos = nIndex >= 0 ? nIndex : cbOldSize + nIndex;
		// 插入位置不能超出当前已写入的数据范围
		if (iWritePos > cbOldSize)
			return false;
		// 申请足够的内存空间
		void *p = GetBuffer(cbOldSize + cbSize);
		if (p == NULL)
			return false;
		// 移动现有数据并写入新的数据
		memmove((uint8_t *)p + iWritePos + cbSize, (uint8_t *)p + iWritePos, cbOldSize - iWritePos);
		memcpy((uint8_t *)p + iWritePos, pData, cbSize);
		return true;
	}

	// 从指定位置开始删除指定长度的数据
	bool Delete(int nIndex, size_t cbDeleteSize)
	{
		size_t cbOldSize = GetSize();
		size_t iDeletePos = nIndex >= 0 ? nIndex : cbOldSize + nIndex;
		// 删除位置不能超出当前已写入的数据范围
		if (iDeletePos > cbOldSize)
			return false;
		// 要删除的数据长度如果超出缓冲区结尾就删除到缓冲区结尾部分的数据
		if (iDeletePos + cbDeleteSize >= cbOldSize)
			cbDeleteSize = cbOldSize - cbDeleteSize;
		// 删除数据然后重置缓冲区长度
		char *p = (char *)GetBuffer(cbOldSize);
		memcpy(p + iDeletePos, p + iDeletePos + cbDeleteSize, cbOldSize - (iDeletePos + cbDeleteSize));
		ReleaseBuffer(cbOldSize - cbDeleteSize);
		return true;
	}

	void Empty()
	{
		ReleaseBuffer(0);
	}

	const void *GetBuffer() const
	{
		if (m_pBuffer == NULL || m_pBuffer->Length == 0)
			return NULL;
		return m_pBuffer->Buffer;
	}

	size_t GetSize() const
	{
		if (m_pBuffer == NULL || m_pBuffer->Buffer == NULL)
			return 0;
		return m_pBuffer->Length;
	}

	bool IsEmpty() const
	{
		return GetSize() == 0;
	}

	operator const void *() const
	{
		return GetBuffer();
	}

protected:
	CBuffer &CopyOf(const CBuffer &C)
	{
		if (this != &C)
		{
			DestoryBuffer();
			if (C.m_pBuffer)
				C.m_pBuffer->RefCount++;
			m_pBuffer = C.m_pBuffer;
		}
		return *this;
	}

public:
	CBuffer &operator=(const CBuffer &C)
	{
		return CopyOf(C);
	}

	CBuffer(const CBuffer &C)
	{
		if (C.m_pBuffer)
			C.m_pBuffer->RefCount++;
		m_pBuffer = C.m_pBuffer;
	}

	CBuffer()
	{
		m_pBuffer = NULL;
	}

	virtual ~CBuffer()
	{
		DestoryBuffer();
	}

private:
	PBUFFER m_pBuffer;

	void CreateBuffer()
	{
		if (m_pBuffer)
			DestoryBuffer();

		m_pBuffer = new BUFFER;
		m_pBuffer->RefCount = 1;
		m_pBuffer->AllocLength = 0;
		m_pBuffer->Length = 0;
		m_pBuffer->Buffer = NULL;
	}

	PBUFFER CopyBuffer(size_t cbSize = ~0U)
	{
		if (cbSize == -1)
			cbSize = m_pBuffer->Length;

		PBUFFER pBuffer = new BUFFER;
		pBuffer->RefCount = 1;
		pBuffer->AllocLength = cbSize + MEM_ALLOC_RESERVE_SIZE;
		pBuffer->Length = min(cbSize, m_pBuffer->Length);
		pBuffer->Buffer = (uint8_t *)malloc(pBuffer->AllocLength);

		if (pBuffer->Buffer == NULL)
		{
			delete pBuffer;
			return NULL;
		}
		memcpy(pBuffer->Buffer, m_pBuffer->Buffer, pBuffer->Length);
		return pBuffer;
	}

	void DestoryBuffer()
	{
		if (m_pBuffer)
		{
			if (--m_pBuffer->RefCount == 0)
			{
				if (m_pBuffer->Buffer)
					free(m_pBuffer->Buffer);
				delete m_pBuffer;
			}
			m_pBuffer = NULL;
		}
	}
};

typedef enum {
	BDT_BOOL,
	BDT_INT,
	BDT_UINT,
	BDT_FLOAT,
	BDT_STR,
	BDT_BIN,
} BUFFER_DATA_TYPE;

typedef struct {
	int type : 8;
	uint32_t datalen : 24;
	uint8_t data[0];
} BUFFER_DATA_HEAD;

class BufferWriter
{
public:
	void WriteBool(bool val) {WriteData(BDT_BOOL, &val, sizeof(val));}
	void WriteInt(int val) {WriteData(BDT_INT, &val, sizeof(int));}
	void WriteUint(uint32_t val) {WriteData(BDT_UINT, &val, sizeof(uint32_t));}
	void WriteInt64(int64_t val) {WriteData(BDT_INT, &val, sizeof(val));}
	void WriteUint64(uint64_t val) {WriteData(BDT_UINT, &val, sizeof(val));}
	void WriteFloat(float val) {WriteData(BDT_FLOAT, &val, sizeof(val));}
	void WriteDouble(double val) {WriteData(BDT_FLOAT, &val, sizeof(val));}
	void WriteBin(const void *data, uint32_t len) {WriteData(BDT_BIN, data, len);}
	void WriteStr(const char *str, uint32_t len) {WriteData(BDT_STR, str, len);}
	void WriteStr(const char *str) {WriteStr(str, strlen(str));}

	operator const CBuffer &() { return m_buf; }

	BufferWriter() {}
	virtual ~BufferWriter() {}

protected:
	virtual const CBuffer &GetBuf() const { return m_buf; }

private:
	CBuffer m_buf;

	void WriteData(int type, const void *data, uint32_t datalen)
	{
		// 数据写入需按照4字节的内存边界对齐，所以实际分配的空间为4的倍数
		uint32_t alignlen = (datalen + (type == BDT_STR) + 3) & ~3;
		BUFFER_DATA_HEAD *bdh = (BUFFER_DATA_HEAD *)m_buf.GetBufferEnd(sizeof(BUFFER_DATA_HEAD) + alignlen);
		bdh->type = type;
		bdh->datalen = datalen;
		memcpy(bdh->data, data, datalen);
		memset(bdh->data + datalen, 0, alignlen - datalen);
	}
};

class BufferReader
{
public:
	bool ReadBool() {return *(bool *)GetDataPtr(BDT_BOOL, sizeof(bool));}
	int ReadInt() {return *(int *)GetDataPtr(BDT_INT, sizeof(int));}
	uint32_t ReadUint() {return *(uint32_t *)GetDataPtr(BDT_UINT, sizeof(uint32_t));}
	float ReadFloat() {return *(float *)GetDataPtr(BDT_FLOAT, sizeof(float));}
	const void *ReadBin(uint32_t &len) {return GetDataPtr(BDT_BIN, &len);}
	const char *ReadStr(uint32_t &len) {return (char *)GetDataPtr(BDT_STR, &len);}
	const char *ReadStr() {uint32_t len;return (char *)GetDataPtr(BDT_STR, &len);}

	int64_t ReadInt64()
	{
		uint32_t *p = (uint32_t *)GetDataPtr(BDT_INT, sizeof(int64_t));
		return (uint64_t)p[0] | ((uint64_t)p[1] << 32);
	}

	uint64_t ReadUint64()
	{
		uint32_t *p = (uint32_t *)GetDataPtr(BDT_UINT, sizeof(uint64_t));
		return (uint64_t)p[0] | ((uint64_t)p[1] << 32);
	}

	double ReadDouble()
	{
		uint32_t *p = (uint32_t *)GetDataPtr(BDT_FLOAT, sizeof(double));
		uint64_t tmp = (uint64_t)p[0] | ((uint64_t)p[1] << 32);
		return *(double *)&tmp;
	}

	BufferReader(const CBuffer &buf)
	{
		SetBuf(buf);
	}

	virtual ~BufferReader() {};

protected:
	BufferReader()
	{
		m_read_ptr = NULL;
		m_read_ptr_end = NULL;
	}

	void SetBuf(const CBuffer &buf)
	{
		m_buf = buf;
		m_read_ptr = (uint8_t *)m_buf.GetBuffer();
		m_read_ptr_end = m_read_ptr + m_buf.GetSize();
	}

private:
	CBuffer m_buf;
	uint8_t *m_read_ptr;
	uint8_t *m_read_ptr_end;

	void *GetDataPtr(int type, uint32_t datalen)
	{
		BUFFER_DATA_HEAD *bdh = (BUFFER_DATA_HEAD *)m_read_ptr;
		assert(m_read_ptr + sizeof(BUFFER_DATA_HEAD) <= m_read_ptr_end);
		assert(bdh->type == type);
		assert(bdh->datalen == datalen);
		uint32_t alignlen = (bdh->datalen + 3) & ~3;
		m_read_ptr += sizeof(BUFFER_DATA_HEAD) + alignlen;
		return bdh->data;
	}

	void *GetDataPtr(int type, uint32_t *datalen)
	{
		BUFFER_DATA_HEAD *bdh = (BUFFER_DATA_HEAD *)m_read_ptr;
		assert(m_read_ptr + sizeof(BUFFER_DATA_HEAD) <= m_read_ptr_end);
		assert(bdh->type == type);
		*datalen = bdh->datalen;
		uint32_t alignlen = (bdh->datalen + (type == BDT_STR) + 3) & ~3;
		m_read_ptr += sizeof(BUFFER_DATA_HEAD) + alignlen;
		return bdh->data;
	}
};

#endif