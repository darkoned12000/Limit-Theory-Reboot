#include "LTE/Function.h"
#include "LTE/FunctionBind.h"
#include "LTE/Keyboard.h"
#include "LTE/Vector.h"

namespace Priv1 {
  #define X(x)                                                                   \
    static Function const Key_##x##_Registration = Function_Bind(               \
      "Key_" #x,                                                                 \
      "Return the " #x " key",                                                   \
      []() -> Key { return Key_##x; });
  KEY_X
  #undef X
}

static Function const Key_Alt_Registration = Function_Bind(
  "Key_Alt",
  "Return whether an alt key is down",
  []() -> bool
  {
  return Keyboard_Alt();
  });

static Function const Key_Control_Registration = Function_Bind(
  "Key_Control",
  "Return whether a control key is down",
  []() -> bool
  {
  return Keyboard_Control();
  });

static Function const Key_Down_Registration = Function_Bind(
  "Key_Down",
  "Return whether 'key' is currently down",
  [](Key const& key) -> bool
  {
  return Keyboard_Down(key);
  },
  "key");
static int const Key_Down_Alias = Function_Alias("Key_Down", "Down");

static Function const Keyboard_GetPressed_Registration = Function_Bind(
  "Keyboard_GetPressed",
  "Return a list of all keys pressed this frame",
  []() -> Vector<Key>
  {
  return Keyboard_GetKeysPressed();
  });

static Function const Keyboard_ModifyString_Registration = Function_Bind(
  "Keyboard_ModifyString",
  "Modify the 's' according to keys pressed this frame",
  [](String const& s, int const& cursor)
  {
  Keyboard_ModifyString(Mutable(s), Mutable(cursor));
  },
  "s", "cursor");

static Function const Key_Pressed_Registration = Function_Bind(
  "Key_Pressed",
  "Return whether 'key' was pressed this frame",
  [](Key const& key) -> bool
  {
  return Keyboard_Pressed(key);
  },
  "key");
static int const Key_Pressed_Alias = Function_Alias("Key_Pressed", "Pressed");

static Function const Key_Released_Registration = Function_Bind(
  "Key_Released",
  "Return whether 'key' was released this frame",
  [](Key const& key) -> bool
  {
  return Keyboard_Released(key);
  },
  "key");
static int const Key_Released_Alias = Function_Alias("Key_Released", "Released");

static Function const Key_Shift_Registration = Function_Bind(
  "Key_Shift",
  "Return whether a shift key is down",
  []() -> bool
  {
  return Keyboard_Shift();
  });
