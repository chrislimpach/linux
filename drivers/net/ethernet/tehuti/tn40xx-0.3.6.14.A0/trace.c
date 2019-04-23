//
//	trace.c
//

//#include <linux/hrtimer.h>

#include "tn40.h"

#define TRACE_SIZE	32 // Must be **2
#define TRACE_MASK  (TRACE_SIZE -1)

typedef struct
{
	u64			loc;
	u64			nSec;
	u64			val;
} traceLine_t;

typedef struct
{
	u32			traceOn;
	u32			wrap;
	u32 		line;
	traceLine_t	lines[TRACE_SIZE];
} traceLines_t;

traceLines_t		traceLines;

//-------------------------------------------------------------------------------------------------

void traceInit(void)
{
	memset(&traceLines, 0, sizeof(traceLines));

} // traceInit


//-------------------------------------------------------------------------------------------------

void traceOn(void)
{
	traceLines.traceOn = 1;
	traceLines.wrap    = 1;

} // traceOn

//-------------------------------------------------------------------------------------------------

void traceOff(void)
{
	traceLines.traceOn = 0;
	traceLines.wrap    = 0;

} // traceOn

//-------------------------------------------------------------------------------------------------

void traceOnce(void)
{
	traceLines.traceOn = 1;
	traceLines.wrap    = 0;
	traceLines.line    = 0;


} // traceOn

//-------------------------------------------------------------------------------------------------

void traceAddX(u64 loc, u64 val)
{
	if (traceLines.traceOn)
	{
		if ((!traceLines.wrap) && (traceLines.line == TRACE_MASK))
		{
			traceLines.traceOn = 0;
			MSG("traceAdd() trace turned off at line %d\n", traceLines.line);
		}
		else
		{
			traceLines.lines[traceLines.line].nSec 	= ktime_get().tv64;
			traceLines.lines[traceLines.line].loc  	= loc;
			traceLines.lines[traceLines.line].val  	= val;
			traceLines.line 						= (traceLines.line + 1) & TRACE_MASK;
			traceLines.lines[traceLines.line].loc  	= -1;
		}
	}

} // traceAdd

//-------------------------------------------------------------------------------------------------

void traceAdd(u64 loc, u64 val)
{
	if (traceLines.traceOn)
	{
		traceLines.lines[traceLines.line].nSec 	= ktime_get().tv64;
		traceLines.lines[traceLines.line].loc  	= loc;
		traceLines.lines[traceLines.line].val  	= val;
		if ((!traceLines.wrap) && (traceLines.line == TRACE_MASK))
		{
			traceLines.traceOn = 0;
			MSG("traceAdd() trace turned off at line %d\n", traceLines.line);
		}
		else
		{
			traceLines.line = (traceLines.line + 1) & TRACE_MASK;
		}
	}

} // traceAdd

//-------------------------------------------------------------------------------------------------

void tracePrint(void)
{

	int j, line;

	MSG("tracePrint() at line %d on=%d wrap=%d\n", traceLines.line, traceLines.traceOn, traceLines.wrap);

	line = (traceLines.line +1) & TRACE_MASK;
	for (j = 0; j < TRACE_SIZE; j++)
	{
		MSG("%llx, %lld, %lld\n", traceLines.lines[line].loc, traceLines.lines[line].nSec, traceLines.lines[line].val);
		line = (line +1) & TRACE_MASK;
	}

} // tracePrint()

//-------------------------------------------------------------------------------------------------

