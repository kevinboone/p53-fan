/*=============================================================================

  p53-fan
  fan.c
  Copyright (c)2025 Kevin Boone, GPL3.0

=============================================================================*/

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>
#include "config.h" 
#include "mylog.h" 
#include "fan.h" 

/** 
  fan_write
  Write a string, then a terminating \n, to the fan control
  pseudo-file /proc/acpi/ibm/fan.
*/
static int fan_write (const char *text, BOOL dry_run)
  {
  mylog_trace ("Called fan_write with '%s'", text);
  if (dry_run)
    {
    return 0;
    }
  char s[32];
  strcpy (s, text);
  strcat (s, "\n");
  int ret = -1;
  int f = open (FAN_FILE, O_WRONLY);
  if (f >= 0)
    {
    int n = write (f, s, strlen (s));
    mylog_trace ("write() returned %d", n);
    close (f);
    ret = 0;
    }
  else
    {
    ret = -1;
    mylog_error ("Can't open '%s' for writing", FAN_FILE);
    }
  return ret;
  }

/**
  fan_set_level
  Set the fan level from 0-8. We do this by writing 'level N' to the fan
control pseudo-file. Level 8, however, is an interval level used by this
program, and not the fan driver. We translate level 8 to 'disengaged'.
*/
void fan_set_level (int new_level, BOOL dry_run)
  {
  mylog_debug ("Setting fan level %d", new_level);
  if (new_level == FAN_MAX)
    fan_write ("level disengaged", dry_run);
  else
    {
    char s[32];
    snprintf (s, sizeof (s), "level %d", new_level);
    fan_write (s, dry_run);
    }
  }

/**
  fan_to_auto 
  On program exit, return the fan control to its default, automatic mode.
*/
int fan_to_auto (BOOL dry_run)
  {
  int ret = 0;
  mylog_debug ("Trying to set fan to automatic");
  ret |= fan_write ("level auto", dry_run);
  if (ret == 0) ret |= fan_write ("enable", dry_run);
  if (ret)
    mylog_error ("Can't set fan to automatic");
  else
    mylog_info ("Fan control enabled");
  return ret;
  }

/**
  fan_to_manual
  At start-up, set the fan control driver to programmatic mode. Note that this
will fail if the kernel module did not have fan control enabled at boot time,
even if the user has permissions to write the relevant pseudo-file.
*/
int fan_to_manual (BOOL dry_run)
  {
  mylog_debug ("Trying to set fan to programatic");
  int ret = 0;
  ret |= fan_write ("disable", dry_run);
  // We have to set some initial fan speed, and there's no way to know what it
  //   should be at this point. We can't just say "auto", as this re-enables
  //   default fan control.
  if (ret == 0) ret |= fan_write ("level 3", dry_run);
  if (ret)
    mylog_error ("Can't set fan to programatic");
  else
    mylog_info ("Fan control enabled");
  return ret;
  }


