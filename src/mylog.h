/*=============================================================================

  p53-fan
  error.h 
  Copyright (c)2025 Kevin Boone, GPL3.0

=============================================================================*/

#pragma once

#include "defs.h"

#define MYLOG_ERROR 0
#define MYLOG_WARN 1
#define MYLOG_INFO 2
#define MYLOG_DEBUG 3
#define MYLOG_TRACE 4

extern int mylog_level;
extern BOOL mylog_syslog;

extern void mylog_warn (const char *fmt,...);
extern void mylog_error (const char *fmt,...);
extern void mylog_info (const char *fmt,...);
extern void mylog_debug (const char *fmt,...);
extern void mylog_trace (const char *fmt,...);
extern void mylog (int level, const char *fmt,...);

