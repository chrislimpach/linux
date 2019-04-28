//
//	memLog.c
//
#include "memLog.h"

#define MAX(a, b)   	((a) > (b) ? (a) : (b))

typedef struct
{
	size_t 			printNext;
	long long 		printSize;
	int				wrapped;
	size_t			nextMsg;
	char			buf[MEMLOG_SIZE];
} memLogBuf_t;

memLogBuf_t		memLogBuf;

//-------------------------------------------------------------------------------------------------

void memLogInit(void)
{
	memset((void*)&memLogBuf, 0, sizeof(memLogBuf));

} // traceInit

//-------------------------------------------------------------------------------------------------

void memLog(const char *fmt, ...)
{
	int	 	maxlen, bytes;
    va_list args, args1;

    va_start(args, fmt);
	va_copy(args1, args);

	maxlen = (int)(MEMLOG_SIZE - memLogBuf.nextMsg);
	bytes  = vsnprintf(&memLogBuf.buf[memLogBuf.nextMsg], maxlen, fmt, args) + 1;
	if (bytes > maxlen)											// no room, wrap
	{
		memset(&memLogBuf.buf[memLogBuf.nextMsg],0,maxlen);		// pad with '\0'
		memLogBuf.wrapped = 1;
		memLogBuf.nextMsg = 0;
		bytes             = vsnprintf(&memLogBuf.buf[memLogBuf.nextMsg], MEMLOG_SIZE, fmt, args1) + 1;
	}
	memLogBuf.nextMsg 	 += bytes;

 	va_end(args);
	va_end(args1);

} // memLog

//-------------------------------------------------------------------------------------------------

void memLogDmesg(void)
{
	int 		bytes, maxlen;
	size_t 		next;
	long long 	logSize;

	if (memLogBuf.wrapped)
	{
		next 	= memLogBuf.nextMsg;
		logSize = MEMLOG_SIZE;
	}
	else
	{
		next 	= 0;
		logSize = memLogBuf.nextMsg;
	}
	MSG("Memlog buffer:\n");
	while (logSize > 0)
	{
		maxlen = (int)(MEMLOG_SIZE - next);
		bytes  = (int)strnlen(&memLogBuf.buf[next], maxlen);
		MSG("%s", &memLogBuf.buf[next]);
		if ((bytes) && (memLogBuf.buf[next + bytes -1] != '\n'))
		{
			MSG("\n");
		}
		bytes += 1;						// '\0'
		next  += bytes;
		if (next >= MEMLOG_SIZE)		// wrap
		{
			next = 0;
		}
		logSize -= bytes;
		msleep(4);
	}
	memLogBuf.wrapped = 0;
	memLogBuf.nextMsg = 0;

} // memLogDmesg

//-------------------------------------------------------------------------------------------------

char *memLogGetLine(uint *buf_size)
{
	int 		maxlen;
	int 		bytes = 0;
	char		*buf  = NULL;

	DBG("printSize %lld\n", memLogBuf.printSize);
	if (memLogBuf.printSize == 0)
	{
		if (memLogBuf.wrapped)
		{
			memLogBuf.printNext = memLogBuf.nextMsg;
			memLogBuf.printSize = MEMLOG_SIZE;
		}
		else
		{
			memLogBuf.printNext = 0;
			memLogBuf.printSize = memLogBuf.nextMsg;
		}
	}
	DBG("printSize %lld printNext %ld\n", memLogBuf.printSize, memLogBuf.printNext);
	if (memLogBuf.printSize > 0)
	{
		maxlen = (int)(MEMLOG_SIZE - memLogBuf.printNext);
		bytes  = (int)strnlen(&memLogBuf.buf[memLogBuf.printNext], maxlen);
		*buf_size = bytes;
		bytes += 1; // '\0'
		buf    = &memLogBuf.buf[memLogBuf.printNext];
		memLogBuf.printNext  += bytes;
		if (memLogBuf.printNext >= MEMLOG_SIZE)		// wrap
		{
			memLogBuf.printNext = 0;
		}
		memLogBuf.printSize -= bytes;
	}
	if (memLogBuf.printSize == 0)
	{
		memLogBuf.wrapped = 0;
		memLogBuf.nextMsg = 0;
	}
	DBG("return 0x%p\n", buf);
	return buf;

} // memLogDmesg
