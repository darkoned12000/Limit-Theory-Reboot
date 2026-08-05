#include "FrameTimer.h"

#include "LTE/Module.h"
#include "LTE/Timer.h"
#include "LTE/FunctionBind.h"

namespace {
  struct FrameTimerModule : public ModuleT {
    Timer timer;
    float dt;
    float ema01;
    float ema1;
    float ema10;

    FrameTimerModule() :
      dt(0),
      ema01(0),
      ema1(0),
      ema10(0)
      {}

    char const* GetName() const override {
      return "Frame Timer";
    }

    void Update() override {
      dt = timer.GetElapsed();
      ema01 = Mix(ema01, dt, 1.0 - Exp(-dt / 0.5f));
      ema1 = Mix(ema1, dt, 1.0 - Exp(-dt));
      ema10 = Mix(ema10, dt, 1.0 - Exp(-dt / 10.0f));
      timer.Reset();
    }
  };

  FrameTimerModule* FrameTimer_GetModule() {
    static Reference<FrameTimerModule> module;
    if (!module) {
      module = new FrameTimerModule;
      Module_RegisterGlobal(module);
    }
    return module;
  }
}

float FrameTimer_Get() {
  return FrameTimer_GetModule()->dt;
}
static Function const FrameTimer_Get_Registration = Function_Bind(
  "FrameTimer_Get",
  "None",
  &FrameTimer_Get);



float FrameTimer_GetEMA01() {
  return FrameTimer_GetModule()->ema01;
}
static Function const FrameTimer_GetEMA01_Registration = Function_Bind(
  "FrameTimer_GetEMA01",
  "None",
  &FrameTimer_GetEMA01);



float FrameTimer_GetEMA1() {
  return FrameTimer_GetModule()->ema1;
}
static Function const FrameTimer_GetEMA1_Registration = Function_Bind(
  "FrameTimer_GetEMA1",
  "None",
  &FrameTimer_GetEMA1);



float FrameTimer_GetEMA10() {
  return FrameTimer_GetModule()->ema10;
}
static Function const FrameTimer_GetEMA10_Registration = Function_Bind(
  "FrameTimer_GetEMA10",
  "None",
  &FrameTimer_GetEMA10);


