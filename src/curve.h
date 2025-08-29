/*=============================================================================

  p53-fan
  curve.h
  Copyright (c)2025 Kevin Boone, GPL3.0

=============================================================================*/

#pragma once

// Constants defining the fan curve number to use
typedef enum _CurveNum
  {
  CURVE_COLD=0,
  CURVE_COOL=1,
  CURVE_MEDIUM=2,
  CURVE_WARM=3,
  CURVE_HOT=4
  } CurveNum;

extern int curve_get_level (CurveNum curve_num, int old_level, int temp);
extern const char *curve_get_name (CurveNum curve_num);
