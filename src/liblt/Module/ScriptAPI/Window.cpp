// Copyright (C) 2025  darkoned12000
// SPDX-License-Identifier: GPL-3.0-or-later
// Part of the ltheory-old-test modernization effort (Revamp Work).
// See NOTICE and LICENSE.GPL. Original engine (c) Josh Parnell, public domain.

#include "LTE/Function.h"
#include "LTE/Window.h"

FreeFunctionNoParams(bool, Window_GetFullscreen,
  "Return true if the window is in fullscreen mode")
{
  return Window_Get() ? Window_Get()->GetFullscreen() : false;
}

VoidFreeFunction(Window_SetFullscreen,
  "Switch between fullscreen and windowed mode. "
  "NOTE: switching to/from fullscreen recreates the window and loses all "
  "OpenGL state (shaders, textures, VBOs). Call Shader_RecompileAll after.",
  bool, fullscreen)
{
  Window win = Window_Get();
  if (win)
    win->SetFullscreen(fullscreen);
}

FreeFunctionNoParams(int, Window_GetWidth,
  "Return the current window width in pixels")
{
  return Window_Get() ? (int)Window_Get()->GetSize().x : 0;
}

FreeFunctionNoParams(int, Window_GetHeight,
  "Return the current window height in pixels")
{
  return Window_Get() ? (int)Window_Get()->GetSize().y : 0;
}

VoidFreeFunction(Window_SetSize,
  "Set the window resolution to 'width' x 'height' pixels. "
  "NOTE: when in fullscreen this recreates the window and loses all "
  "OpenGL state. In windowed mode, this safely resizes in-place.",
  int, width,
  int, height)
{
  Window win = Window_Get();
  if (win)
    win->SetSize(V2U((uint)width, (uint)height));
}

FreeFunctionNoParams(bool, Window_GetVSync,
  "Return true if vertical sync is enabled")
{
  return Window_Get() ? Window_Get()->GetVSync() : false;
}

VoidFreeFunction(Window_SetVSync,
  "Enable or disable vertical sync",
  bool, enabled)
{
  Window win = Window_Get();
  if (win)
    win->SetSync(enabled);
}
