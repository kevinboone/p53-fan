/*=============================================================================

  p53-fan
  hwmon_scan.h 
  Copyright (c)2025 Kevin Boone, GPL3.0

=============================================================================*/

#pragma once

#include "defs.h"

typedef struct _HSContext
  {
  int max_temp;
  char driver[32];
  char label[32];
  char path[256];
  BOOL nowifi;
  BOOL nodrivetemp;
  BOOL valid; 
  } HSContext; 

extern int hwmon_scan (HSContext *context, BOOL nowifi, BOOL nodrivetemp); 


