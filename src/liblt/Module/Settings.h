#ifndef Module_Settings_h__
#define Module_Settings_h__

#include "LTE/Common.h"
#include "LTE/Generic.h"
#include "LTE/String.h"
#include "UI/Common.h"
#include "UI/Widget.h"

LT_API GenericAxis Settings_Axis(
  String const& name,
  Axis const& defValue);

LT_API GenericBool Settings_Bool(
  String const& name,
  bool defValue);

LT_API GenericButton Settings_Button(
  String const& name,
  Button const& defValue);

LT_API GenericColor Settings_Color(
  String const& name,
  Color const& defValue);

LT_API GenericFloat Settings_Float(
  String const& name,
  float minimum,
  float maximum,
  float defValue);

LT_API Widget Widget_Settings();

LT_API GenericColor Settings_PrimaryColor();

LT_API GenericColor Settings_SecondaryColor();

#endif
