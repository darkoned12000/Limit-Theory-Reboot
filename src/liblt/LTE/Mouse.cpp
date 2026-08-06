#include "Mouse.h"
#include "Timer.h"
#include "Window.h"

#include "Module/FrameTimer.h"

#include "SFML/Graphics.hpp"
#include "LTE/FunctionBind.h"

const float kDoubleClickThresh = 0.1f;

namespace {
  struct Mouse {
    int x;
    int y;
    int lastX;
    int lastY;
    float lastClickInterval;
    float scrollDelta;
    float idleTime;

    Timer downTimer[MouseButton_SIZE];
    bool down[MouseButton_SIZE];
    bool lastDown[MouseButton_SIZE];

    Timer releaseTimer[MouseButton_SIZE];

    Mouse() :
      x(0),
      y(0),
      lastX(0),
      lastY(0),
      scrollDelta(0),
      idleTime(0)
    {
      for (MouseButton i = 0; i < MouseButton_SIZE; ++i) {
        down[i] = false;
        lastDown[i] = false;
      }
    }
  } gMouse;
}

namespace LTE {
  bool Mouse_DoubleClicked() {
    return Mouse_LeftPressed()
      && gMouse.releaseTimer[MouseButton_Left].GetElapsed() < kDoubleClickThresh;
  }
static Function const Mouse_DoubleClicked_Registration = Function_Bind(
  "Mouse_DoubleClicked",
  "None",
  &Mouse_DoubleClicked);



  bool Mouse_Down(MouseButton button) {
    return gMouse.down[button];
  }

  bool Mouse_Pressed(MouseButton button) {
    return gMouse.down[button] && !gMouse.lastDown[button];
  }

  bool Mouse_Released(MouseButton button) {
    return !gMouse.down[button] && gMouse.lastDown[button];
  }

  float Mouse_GetDownTime(MouseButton button) {
    return gMouse.downTimer[button].GetElapsed();
  }

  float Mouse_GetScrollDelta() {
    return gMouse.scrollDelta;
  }
static Function const Mouse_GetScrollDelta_Registration = Function_Bind(
  "Mouse_GetScrollDelta",
  "None",
  &Mouse_GetScrollDelta);



  V2 Mouse_GetDP() {
    return V2(
      (float)(gMouse.x - gMouse.lastX),
      (float)(gMouse.y - gMouse.lastY));
  }
static Function const Mouse_GetDP_Registration = Function_Bind(
  "Mouse_GetDP",
  "None",
  &Mouse_GetDP);



  float Mouse_GetIdleTime() {
    return gMouse.idleTime;
  }
static Function const Mouse_GetIdleTime_Registration = Function_Bind(
  "Mouse_GetIdleTime",
  "None",
  &Mouse_GetIdleTime);



  int Mouse_GetDX() {
    return gMouse.x - gMouse.lastX;
  }
static Function const Mouse_GetDX_Registration = Function_Bind(
  "Mouse_GetDX",
  "None",
  &Mouse_GetDX);



  int Mouse_GetDY() {
    return gMouse.y - gMouse.lastY;
  }
static Function const Mouse_GetDY_Registration = Function_Bind(
  "Mouse_GetDY",
  "None",
  &Mouse_GetDY);



  V2 Mouse_GetPos() {
    return V2((float)gMouse.x, (float)gMouse.y);
  }
static Function const Mouse_GetPos_Registration = Function_Bind(
  "Mouse_GetPos",
  "None",
  &Mouse_GetPos);



  V2 Mouse_GetPosImmediate() {
    sf::Vector2i p = sf::Mouse::getPosition(
      *(sf::RenderWindow*)Window_Get()->GetImplData());
    return V2((float)p.x, (float)p.y);
  }
static Function const Mouse_GetPosImmediate_Registration = Function_Bind(
  "Mouse_GetPosImmediate",
  "None",
  &Mouse_GetPosImmediate);



  V2 Mouse_GetPosLast() {
    return V2((float)gMouse.lastX, (float)gMouse.lastY);
  }
static Function const Mouse_GetPosLast_Registration = Function_Bind(
  "Mouse_GetPosLast",
  "None",
  &Mouse_GetPosLast);



  int Mouse_GetX() {
    return gMouse.x;
  }
static Function const Mouse_GetX_Registration = Function_Bind(
  "Mouse_GetX",
  "None",
  &Mouse_GetX);



  int Mouse_GetY() {
    return gMouse.y;
  }
static Function const Mouse_GetY_Registration = Function_Bind(
  "Mouse_GetY",
  "None",
  &Mouse_GetY);



  void Mouse_SetPos(V2 const& v) {
    gMouse.x = (int)v.x;
    gMouse.y = (int)v.y;
    sf::Mouse::setPosition(
      sf::Vector2i(gMouse.x, gMouse.y),
      *(sf::RenderWindow*)Window_Get()->GetImplData());
  }

  void Mouse_SetPressed(MouseButton button, bool pressed) {
    gMouse.down[button] = pressed;
  }

  void Mouse_SetScrollDelta(float ds) {
    gMouse.scrollDelta = ds;
  }

  void Mouse_Update() {
    gMouse.idleTime += FrameTimer_Get();
    if (gMouse.lastX != gMouse.x || gMouse.lastY != gMouse.y)
      gMouse.idleTime = 0;
    gMouse.lastX = gMouse.x;
    gMouse.lastY = gMouse.y;

    for (MouseButton button = 0; button < MouseButton_SIZE; ++button) {
      if (gMouse.down[button] && !gMouse.lastDown[button])
        gMouse.downTimer[button].Reset();
      if (!gMouse.down[button] && gMouse.lastDown[button])
        gMouse.releaseTimer[button].Reset();
      gMouse.lastDown[button] = gMouse.down[button];
    }

    gMouse.scrollDelta = 0;
  }

  void Mouse_UpdatePos(V2I const& p) {
    gMouse.x = p.x;
    gMouse.y = p.y;
  }

  bool Mouse_LeftDown() {
    return Mouse_Down(MouseButton_Left);
  }
static Function const Mouse_LeftDown_Registration = Function_Bind(
  "Mouse_LeftDown",
  "None",
  &Mouse_LeftDown);



  bool Mouse_LeftPressed() {
    return Mouse_Pressed(MouseButton_Left);
  }
static Function const Mouse_LeftPressed_Registration = Function_Bind(
  "Mouse_LeftPressed",
  "None",
  &Mouse_LeftPressed);



  bool Mouse_LeftReleased() {
    return Mouse_Released(MouseButton_Left);
  }
static Function const Mouse_LeftReleased_Registration = Function_Bind(
  "Mouse_LeftReleased",
  "None",
  &Mouse_LeftReleased);



  bool Mouse_RightDown() {
    return Mouse_Down(MouseButton_Right);
  }
static Function const Mouse_RightDown_Registration = Function_Bind(
  "Mouse_RightDown",
  "None",
  &Mouse_RightDown);




  bool Mouse_RightPressed() {
    return Mouse_Pressed(MouseButton_Right);
  }
static Function const Mouse_RightPressed_Registration = Function_Bind(
  "Mouse_RightPressed",
  "None",
  &Mouse_RightPressed);



  bool Mouse_RightReleased() {
    return Mouse_Released(MouseButton_Right);
  }
static Function const Mouse_RightReleased_Registration = Function_Bind(
  "Mouse_RightReleased",
  "None",
  &Mouse_RightReleased);


}
