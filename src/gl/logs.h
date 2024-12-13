#ifndef _GL4ES_LOGS_H_
#define _GL4ES_LOGS_H_
//----------------------------------------------------------------------------
#include <stdio.h>
#include "gl4es.h"
#include "init.h"
//----------------------------------------------------------------------------
void LogPrintf_NoPrefix(const char *fmt,...);
void LogFPrintf(FILE *fp,const char *fmt,...);
void LogPrintf(const char *fmt,...);
//----------------------------------------------------------------------------
#ifdef GL4ES_SILENCE_MESSAGES
	#define SHUT_LOGD(...)
	#define SHUT_LOGD_NOPREFIX(...)
	#define SHUT_LOGE(...)
#else
	#define SHUT_LOGD(...) printf(__VA_ARGS__);printf("\n");write_log(__VA_ARGS__)
	#define SHUT_LOGD_NOPREFIX(...) if(!globals4es.nobanner) LogPrintf_NoPrefix(__VA_ARGS__)
	#define SHUT_LOGE(...) printf(__VA_ARGS__);printf("\n");write_log(__VA_ARGS__)
#endif
//----------------------------------------------------------------------------
#define LOGD(...) LogPrintf(__VA_ARGS__);write_log(__VA_ARGS__)
#define LOGE(...) LogFPrintf(stderr,__VA_ARGS__);write_log(__VA_ARGS__)
//----------------------------------------------------------------------------
#endif // _GL4ES_LOGS_H_
