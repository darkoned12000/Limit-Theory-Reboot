// Copyright (C) 2025  darkoned12000
// SPDX-License-Identifier: GPL-3.0-or-later
// Part of the ltheory-old-test modernization effort (Revamp Work).
// See NOTICE and LICENSE.GPL. Original engine (c) Josh Parnell, public domain.

#include "LTE/Function.h"
#include "LTE/FunctionBind.h"
#include "LTE/Window.h"

static Function const Window_GetFullscreen_Registration = Function_Bind(
  "Window_GetFullscreen",
  "Return true if the window is in fullscreen mode",
  []() -> bool
  {
  return Window_Get() ? Window_Get()->GetFullscreen() : false;
  });

static Function const Window_SetFullscreen_Registration = Function_Bind(
  "Window_SetFullscreen",
  "Switch between fullscreen and windowed mode. "
  "NOTE: switching to/from fullscreen recreates the window and loses all "
  "OpenGL state (shaders, textures, VBOs). Call Shader_RecompileAll after.",
  [](bool const& fullscreen)
  {
  Window win = Window_Get();
  if (win)
    win->SetFullscreen(fullscreen);
  },
  "fullscreen");

static Function const Window_GetWidth_Registration = Function_Bind(
  "Window_GetWidth",
  "Return the current window width in pixels",
  []() -> int
  {
  return Window_Get() ? (int)Window_Get()->GetSize().x : 0;
  });

static Function const Window_GetHeight_Registration = Function_Bind(
  "Window_GetHeight",
  "Return the current window height in pixels",
  []() -> int
  {
  return Window_Get() ? (int)Window_Get()->GetSize().y : 0;
  });

static Function const Window_SetSize_Registration = Function_Bind(
  "Window_SetSize",
  "Set the window resolution to 'width' x 'height' pixels. "
  "NOTE: when in fullscreen this recreates the window and loses all "
  "OpenGL state. In windowed mode, this safely resizes in-place.",
  [](int const& width, int const& height)
  {
  Window win = Window_Get();
  if (win)
    win->SetSize(V2U((uint)width, (uint)height));
  },
  "width", "height");

static Function const Window_GetVSync_Registration = Function_Bind(
  "Window_GetVSync",
  "Return true if vertical sync is enabled",
  []() -> bool
  {
  return Window_Get() ? Window_Get()->GetVSync() : false;
  });

static Function const Window_SetVSync_Registration = Function_Bind(
  "Window_SetVSync",
  "Enable or disable vertical sync",
  [](bool const& enabled)
  {
  Window win = Window_Get();
  if (win)
    win->SetSync(enabled);
  },
  "enabled");
