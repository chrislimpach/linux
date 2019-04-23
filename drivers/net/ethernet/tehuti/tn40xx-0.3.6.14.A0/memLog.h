#ifndef _MEMLOG_H
#define _MEMLOG_H

#include "tn40.h"
#define MEMLOG_SIZE		(128*1024)

void memLog(const char *fmt, ...);
void memLogInit(void);
void memLogDmesg(void);
char *memLogGetLine(uint *buf_size);

#endif
