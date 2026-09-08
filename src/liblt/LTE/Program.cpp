#include "Program.h"

#include "CrashHandler.h"
#include "Joystick.h"
#include "GL.h"
#include "Keyboard.h"
#include "Module.h"
#include "Mouse.h"
#include "StackFrame.h"
#include "Watchdog.h"
#include "Window.h"

#include <ctime>

/* P1-4 watchdog deadlines (ltsl-hardening.md §8/#4, §9). Generous by
   design: only a section that never RETURNS trips — slow frames and
   loading screens return and therefore never fire. A legitimately-ruled
   stall >30s inside a frame or >120s in Initialize is a hang. */
double const kWatchdogFrameSeconds = 10.0;
double const kWatchdogStartupSeconds = 120.0;

namespace  {
  bool destructed = false;
  Program* current = nullptr;
  int exitCode = 0;
}

Program::Program() : deleted(false) {
  srand((uint)time(0));
  CrashHandler_Install();
}

Program::~Program() {
  AUTO_FRAME;
  destructed = true;
}

void Program::Delete() {
  deleted = true;
}

void Program::Execute() {
  Watchdog_Arm(kWatchdogStartupSeconds, "Initialize");
  FRAME("Initialize") {
    Window_Push(window);
    OnInitialize();
    Window_Pop();
  }
  Watchdog_Disarm();

  current = this;
  while (window->IsOpen()) {
    if (deleted) {
      OnDelete();
      break;
    }
    Window_Push(window);

    FRAME("InputUpdate") {
      Mouse_Update();
      Keyboard_Update(window->HasFocus());

      if (window->HasFocus())
        for (uint i = 0; i < Joystick::GetCount(); ++i)
          if (Joystick::Get(i))
            Joystick::Get(i)->Update();
    }

    FRAME("WindowUpdate") {
      Window_Pop();
      window->Update();
      Window_Push(window);
    }

    Watchdog_Arm(kWatchdogFrameSeconds, "Update");
    OnUpdate();
    Watchdog_Disarm();

    Module_UpdateGlobal();

    FRAME("Display")
      window->Display();
    Window_Pop();
  }

  current = nullptr;
}

Program* Program_GetCurrent() {
  return current;
}

void Program_SetExitCode(int code) {
  exitCode = code;
}

int Program_GetExitCode() {
  return exitCode;
}

/* TODO : Fix this ugly mess. */
bool Program_InStaticSection() {
  return destructed;
}
