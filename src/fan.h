/*=============================================================================

  p53-fan
  fan.h 
  Copyright (c)2025 Kevin Boone, GPL3.0

=============================================================================*/

#pragma once

#include "defs.h"

// Note that the thinkpad_acpi driver only accepts levels 0-7. We
// use level '8' internally to represent 'disengaged' fan operation.
#define FAN_MIN 0
#define FAN_MAX 8

extern int fan_to_auto (BOOL dry_run);
extern int fan_to_manual (BOOL dry_run);
extern void fan_set_level (int new_level, BOOL dry_run);

