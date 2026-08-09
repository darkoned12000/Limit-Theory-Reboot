// Copyright (C) 2025  darkoned12000
// SPDX-License-Identifier: GPL-3.0-or-later
// Part of the ltheory-old-test modernization effort (Revamp Work).
// See NOTICE and LICENSE.GPL. Original engine (c) Josh Parnell, public domain.
//
// Modernized version of Josh Parnell's original ltheory.cpp game client.
// Adapted from src/old/ltheory/ltheory.cpp with API fixes for current engine:
//   - SoundEngine_Fmod() -> SoundEngine_SFML()
//   - AddModule() -> Module_RegisterGlobal()
//   - Profiler_AutoProfile() -> Profiler_Auto()
//   - GetInteriorObjects() -> Object_GetInteriorObjects() (free function)
//   - SetInterpolant() removed (no longer exists in engine)
//   - Added override, LTE_Initialize(), OnDelete() cleanup

#include "BuildMode.h"
#ifdef BUILD_DEBUG
#define LTE_CONSOLE
#endif

#include "LTE/LTE.h"

#include "Component/Interior.h"
#include "Component/Resources.h"

#include "Game/Camera.h"
#include "Game/Items.h"
#include "Game/Objects.h"
#include "Game/Player.h"
#include "Game/RenderPasses.h"
#include "Game/Universe.h"
#include "Game/Widgets.h"

#include "Module/FrameTimer.h"
#include "Module/PhysicsEngine.h"
#include "Module/Settings.h"
#include "Module/SoundEngine.h"

#include "UI/Compositors.h"
#include "UI/Interface.h"
#include "UI/Widgets.h"

const uint kUniverseSeed = 1098080;
const Quantity kShipValue = 10000;
const float kSimulationFrequency = 30;

void Screenshot() {
  uint index = 0;
  String prefix = OS_GetUserDataPath() + "screenshot/";
  OS_CreatePath(prefix);
  String path;
  while (true) {
    path = prefix + ToString(index) + ".png";
    if (!OS_FileExists(path))
      break;
    index++;
  }

  Texture_ScreenCapture()->SaveTo(path);
}

struct LTheory : public Program {
  Object universe;
  Player player;
  Camera camera;
  Interface worldView;
  Interface interface;
  float simAccumulator;
  Module physicsEngine;
  Module soundEngine;

  LTheory() : simAccumulator(0) {
    window = Window_Create(
      "Limit Theory",
      V2U(Config_Int("screenwidth", "Graphics", 1280),
          Config_Int("screenheight", "Graphics", 768)),
      true,
      Config_Bool("fullscreen", "Graphics", false));

    window->SetCaptureMouse(true);
    window->SetSync(Config_Bool("enablevsync", "Graphics", true));
    Renderer_Initialize();
  }

  void Reload() {
    if (!interface)
      interface = Interface_Create("UI");
    interface->Clear();
    Script_ClearCache();

    /* Create the primary interface. */ {
      Widget widget;
      ScriptFunction_Load("Widget/HUD:Create")->Call(widget, player);
      LTE_ASSERT(widget);
      interface->Add(widget);
    }
  }

  void OnInitialize() override {
    LTE_Initialize();
    Renderer_Clear();
    window->Display();
    Mouse_SetPos(window->GetSize() / 2);

    Profiler_Auto(5);

    physicsEngine = CreatePhysicsEngine();
    Module_RegisterGlobal(physicsEngine);

    soundEngine = Config_Bool("enableaudio", "General", true)
      ? SoundEngine_SFML()
      : SoundEngine_Null();
    Module_RegisterGlobal(soundEngine);

    Setup();
    Reload();

    /* Create the primary render view. */
    camera = Camera_Create();
    Camera_Push(camera);

    Vector<RenderPass> passes;
    passes.push(RenderPass_Clear(0));
    passes.push(RenderPass_Camera(camera));
    passes.push(RenderPass_SMAA());
    passes.push(RenderPass_Interface(interface));
    passes.push(RenderPass_Bloom(64, 32));
    passes.push(RenderPass_PostFilter("post/dither.jsl"));

    worldView = Interface_Create("Game View");
    worldView->Add(Widget_Rendered(passes));

    Sound_Play2D("system/ambiance/002.wav", 0.02f, true);
    Sound_Play2D("system/ambiance/037.wav", 0.01f, true);
  }

  void OnDelete() override {
    interface = nullptr;
    worldView = nullptr;
    soundEngine = nullptr;
    physicsEngine = nullptr;
  }

  void Setup() {
    uint seed = kUniverseSeed;
    universe = Object_Universe(seed, 1);
    printf("Creating universe %d\n", seed);

    /* Walk down the containment tree to find the System object. */
    Object base = universe;
    while (base && base->GetType() != ObjectType_System) {
      InteriorIterator it = Object_GetInteriorObjects(base);
      if (it.HasMore())
        base = it.Get();
      else
        break;
    }

    if (!base || base->GetType() != ObjectType_System) {
      printf("ERROR: Could not find System in universe\n");
      return;
    }

    /* Ship. */
    Object ship = Item_ShipType(kShipValue, 51)->Instantiate();

    /* Try to start near a station. */
    bool foundStation = false;
    for (InteriorTypeIterator it = Object_GetInteriorObjects(base, ObjectType_Station);
         it.HasMore(); it.Advance())
    {
      Object home = it.Get();
      if (home) {
        ship->SetPos(home->GetPos() + Position(20000, 0, 0));
        foundStation = true;
        break;
      }
    }
    if (!foundStation)
      ship->SetPos(Position(0, 250000, 0));

    base->AddInterior(ship);
    ship->SetLook(Normalize(V3(-0.5f, 0, 0.5f)));

    /* Weapons. */
    {
      Item weapon = Item_WeaponType(40);
      while (ship->Plug(weapon)) {}
      ship->AddItem(weapon, 1);
    }

    /* Random cargo. */
    for (uint i = 0; i < 4; ++i)
      ship->AddItem(base->GetResources()->elements.sample(Rand()), 1);

    /* Player. */ {
      player = Player_Human();
      player->AddCredits(1000);
      player->AddAsset(ship);
      player->Pilot(ship);
      player->SetName("Trent Edison");
    }

    /* Other ship. */ {
      Object ship2 = Item_ShipType(200000000, 49)->Instantiate();
      ship2->Plug(Item_ProductionLabType(1, 1));
      ship2->Plug(Item_TechLabType(1, 1));
      ship2->SetName("Voyager");
      for (uint i = 0; i < 1200; ++i)
        ship2->AddItem(base->GetResources()->elements.sample(Rand()), 1);
      ship2->SetPos(ship->GetPos() + RandDirection() * 1000.0f);
      base->AddInterior(ship2);
      player->AddAsset(ship2);
    }
  }

  void OnUpdate() override {
    float dt = FrameTimer_Get();
    camera->SetTarget(player->piloting);

#ifdef BUILD_DEBUG
    if (Keyboard_Pressed(Key_M))
      player->AddCredits(1000000);
    if (Keyboard_Pressed(Key_K))
      player->piloting->AddItem(Item_WeaponType(rand()), rand() % 3);
    if (Keyboard_Down(Key_H))
      dt *= 10;
    if (Keyboard_Control() && Keyboard_Pressed(Key_W))
      deleted = true;
    if (Keyboard_Pressed(Key_F1))
      Screenshot();
    if (Keyboard_Pressed(Key_F2))
      Profiler_Auto(1.0f);
    if (Keyboard_Pressed(Key_F5))
      Reload();
#endif

    if (player) {
      Object container = player->piloting->GetContainer().t;
      UpdateState state(dt, true);
      state.dt = 1.0f / kSimulationFrequency;

      simAccumulator += kSimulationFrequency * dt;
      while (simAccumulator >= 1.0f) {
        simAccumulator -= 1.0f;
        container->Update(state);
      }

      Universe_Get()->age += state.quanta;
    }

    interface->Update();
    worldView->Update();
    worldView->Draw();
  }
};

int main(int argc, char const* argv[]) {
  LTheory().Execute();
  return 0;
}
