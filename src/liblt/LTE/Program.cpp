#include "Program.h"

#include "CrashHandler.h"
#include "Joystick.h"
#include "GL.h"
#include "Keyboard.h"
#include "Module.h"
#include "Mouse.h"
#include "StackFrame.h"
#include "Window.h"

#include <ctime>

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
  FRAME("Initialize") {
    Window_Push(window);
    OnInitialize();
    Window_Pop();
  }

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

    OnUpdate();

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
