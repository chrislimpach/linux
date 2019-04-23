#ifndef _TRACE_H
#define _TRACE_H

void traceInit(void);
void traceOn(void);
void traceOff(void);
void traceOnce(void);
void tracePrint(void);
void traceAdd(u64 loc, u64 val);

#endif
