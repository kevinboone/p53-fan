/*=============================================================================

  p53-fan
  curve.c
  Copyright (c)2025 Kevin Boone, GPL3.0

=============================================================================*/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "defs.h"
#include "mylog.h"
#include "fan.h"
#include "curve.h"

#define MAX_RANGES 9 

typedef struct _FanRange
  {
  int min;
  int max;
  } FanRange;

/* A fan curve is a set of 9 FanRange elements. Each defines a maximum and
minimum temperature. The position of each element dictates the fan level to
which it corresponds, from 0 to 8. The Thinkpad fan API only accepts 0-7; we
use 8 internally to specify 'disengaged' mode at high temperatures. */

typedef FanRange FanCurve[FAN_MAX + 1];

FanCurve fan_curve_hot = 
  {
  {-273, 56},
  {55, 60},
  {58, 63},
  {61, 66},
  {64, 69},
  {66, 71},
  {71, 76},
  {73, 77},
  {75, 255},
  };

FanCurve fan_curve_warm = 
  {
  {-273, 51},
  {50, 55},
  {53, 58},
  {56, 61},
  {59, 64},
  {62, 67},
  {66, 72},
  {70, 77},
  {75, 255},
  };

FanCurve fan_curve_medium = 
  {
  {-273, 45},
  {43, 50},
  {48, 55},
  {53, 60},
  {58, 65},
  {63, 70},
  {68, 75},
  {73, 77},
  {75, 255},
  };

FanCurve fan_curve_cool = 
  {
  {-273, 45},
  {42, 47},
  {46, 51},
  {50, 55},
  {54, 59},
  {58, 63},
  {62, 67},
  {64, 71},
  {68, 255},
  };

FanCurve fan_curve_cold = 
  {
  {-273, 45},
  {42, 47},
  {44, 49},
  {46, 51},
  {48, 53},
  {51, 56},
  {54, 59},
  {57, 62},
  {60, 255},
  };


/**
  curve_from_number
  Get the curve structure that corresponds to the specific curve number.  The
curve number is in the range 0-4, but it's better to use the constants defined
in curve.h.
*/
static const FanCurve *curve_from_number (CurveNum curve_num)
  {
  switch (curve_num)
    {
    case CURVE_COLD: return &fan_curve_cold;
    case CURVE_COOL: return &fan_curve_cool;
    case CURVE_MEDIUM: return &fan_curve_medium;
    case CURVE_WARM: return &fan_curve_warm;
    case CURVE_HOT: return &fan_curve_hot;
    }
  return NULL; // Should never happen
  }

/**
  get_curve_name
  Return a string representation of the curve with the specified number.
*/
const char *curve_get_name (CurveNum curve_num)
  {
  switch (curve_num)
    {
    case CURVE_COLD: return "cold";
    case CURVE_COOL: return "cool"; 
    case CURVE_MEDIUM: return "medium";
    case CURVE_WARM: return "warm"; 
    case CURVE_HOT: return "hot"; 
    }
  return NULL; // Should never happen
  }

/**
  curve_get_level
  For a given curve, return the fan level that corresponds to the temperature.
A specific temperature can match multiple fan levels, because of hysteresis.
So we first check whether the temperature is a match for the previous level.
If it is, we don't have to change it. Then, if there's no match, we search the
curve _downwards_ from the highest fan level (and thus the highest
temperature). If the temperature appears in multiple fan levels, for safety we
should pick the highest one. 
*/
int curve_get_level (CurveNum curve_num, int old_level, int temp)
  {
  if (old_level < 0 || old_level >= MAX_RANGES)
    {
    mylog_error ("Internal error: fan level is %d ???", old_level);
    return old_level;
    }
 
  const FanCurve *fan_curve = curve_from_number (curve_num); 

  const FanRange *current_range = &((*fan_curve)[old_level]);
  if (temp >= current_range->min && temp < current_range->max)
      {
      mylog_debug ("Current temp %d is in existing range %d-%d, level %d", 
          temp, current_range->min, current_range->max, old_level);
      return old_level; 
      }

  // Search downwards, from higher temperatures to lower
  for (int i = MAX_RANGES - 1; i >= 0; i--)
    {
    const FanRange *test_range = &((*fan_curve)[i]);
    if (temp >= test_range->min && temp < test_range->max)
      {
      mylog_debug ("Current temp is in new range %d-%d, level %d", 
          test_range->min, test_range->max, i);
      return i;
      }
    }

  mylog_error ("Internal error: temp %d is not in any range on curve", temp);
  return old_level;
  }


