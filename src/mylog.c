/*=============================================================================

  p53-fan
  mylog.c
  Copyright (c)2025 Kevin Boone, GPL3.0

=============================================================================*/

#define _GNU_SOURCE 1

#include <syslog.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "defs.h"
#include "mylog.h"

int mylog_level = MYLOG_INFO;
int mylog_syslog = FALSE;

/*==========================================================================
mylog_set_level
*==========================================================================*/
void mylog_set_level (const int level)
  {
  mylog_level = level;
  }

/*==========================================================================
mylog_vprintf
*==========================================================================*/
void mylog_vprintf (const int level, const char *fmt, va_list ap)
  {
  if (level > mylog_level) return;

  if (mylog_syslog)
    {
    switch (level)
      {
      case MYLOG_ERROR: vsyslog (LOG_ERR, fmt, ap); break;
      case MYLOG_WARN: vsyslog (LOG_WARNING, fmt, ap); break;
      case MYLOG_INFO: vsyslog (LOG_NOTICE, fmt, ap); break;
      case MYLOG_DEBUG: vsyslog (LOG_DEBUG, fmt, ap); break;
      case MYLOG_TRACE: /* Ignore */; break;
      }
    }
  else
    {
    char *str = NULL;
    vasprintf (&str, fmt, ap);
    const char *levelstr = "ERROR";
    if (level == MYLOG_WARN)
      levelstr = "WARN";
    else if (level == MYLOG_INFO)
      levelstr = "INFO";
    else if (level == MYLOG_DEBUG)
      levelstr = "DEBUG";
    else if (level == MYLOG_TRACE)
      levelstr = "TRACE";
    fprintf (stderr, APPNAME " %s %s\n", levelstr, str);
    free (str);
    }

  }

/*==========================================================================
mylog_error
*==========================================================================*/
void mylog_error (const char *fmt,...)
  {
  va_list ap;
  va_start (ap, fmt);
  mylog_vprintf (MYLOG_ERROR,  fmt, ap);
  va_end (ap);
  }


/*==========================================================================
mylog_warn
*==========================================================================*/
void mylog_warn (const char *fmt,...)
  {
  va_list ap;
  va_start (ap, fmt);
  mylog_vprintf (MYLOG_WARN,  fmt, ap);
  va_end (ap);
  }


/*==========================================================================
mylog_info
*==========================================================================*/
void mylog_info (const char *fmt,...)
  {
  va_list ap;
  va_start (ap, fmt);
  mylog_vprintf (MYLOG_INFO,  fmt, ap);
  va_end (ap);
  }


/*==========================================================================
mylog_debug
*==========================================================================*/
void mylog_debug (const char *fmt,...)
  {
  va_list ap;
  va_start (ap, fmt);
  mylog_vprintf (MYLOG_DEBUG,  fmt, ap);
  va_end (ap);
  }


/*==========================================================================
mylog_trace
*==========================================================================*/
void mylog_trace (const char *fmt,...)
  {
  va_list ap;
  va_start (ap, fmt);
  mylog_vprintf (MYLOG_TRACE,  fmt, ap);
  va_end (ap);
  }




