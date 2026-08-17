// Copyright (C) 2025  darkoned12000
// SPDX-License-Identifier: GPL-3.0-or-later
// Part of the ltheory-old-test modernization effort (Revamp Work).
// See NOTICE and LICENSE.GPL. Original engine (c) Josh Parnell, public domain.
//
// Runtime regression test for the `this.Method self` call pattern used by
// Widget/SaveGameManager.lts / LoadGameManager.lts to dispatch SAVE / DELETE /
// LOAD from Receive. Invokes the script methods through ScriptFunctionT::Call
// exactly as the widget system does (WidgetCustom::Receive), and verifies the
// target fields are mutated. Uses the fixture at
// resource/script/TestMethodCall.lts.

#include "Harness.h"
#include "Game/SaveGameJSON.h"
#include "LTE/Data.h"
#include "LTE/Field.h"
#include "LTE/Script.h"
#include "LTE/ScriptType.h"
#include "LTE/String.h"
#include "UI/Widget.h"

#include <cstdio>
#include <fstream>
#include <string>

using namespace LTE;

namespace {
  Field const* FindField(ScriptType const& type, String const& name) {
    for (size_t i = 0; i < type->fields.size(); ++i)
      if (type->fields[i].name == name)
        return &type->fields[i];
    return nullptr;
  }

  void* Instance(ScriptType const& type) {
    return type->type->Allocate();
  }

  void SetString(ScriptType const& type, void* instance, String const& field, String const& value) {
    Field const* f = FindField(type, field);
    *(String*)((char*)instance + f->offset) = value;
  }

  String GetString(ScriptType const& type, void* instance, String const& field) {
    Field const* f = FindField(type, field);
    return *(String*)((char*)instance + f->offset);
  }

  int GetInt(ScriptType const& type, void* instance, String const& field) {
    Field const* f = FindField(type, field);
    return *(int*)((char*)instance + f->offset);
  }

  bool GetBool(ScriptType const& type, void* instance, String const& field) {
    Field const* f = FindField(type, field);
    return *(bool*)((char*)instance + f->offset);
  }

  bool FileExists(std::string const& path) {
    std::ifstream f(path.c_str());
    return f.good();
  }

  void CopyFile(std::string const& src, std::string const& dst) {
    std::ifstream in(src.c_str(), std::ios::binary);
    std::ofstream out(dst.c_str(), std::ios::binary);
    out << in.rdbuf();
  }

  /* Drive the manager's Receive exactly as WidgetCustom::Receive does. */
  void Deliver(ScriptType const& type, void* instance, Widget const& w, Data const& msg) {
    ScriptFunction receive = type->GetFunction("Receive");
    if (!receive)
      return;
    receive->VoidCall(0,
      DataRef(type->type, instance),
      DataRef(Type_Get<Widget>(), (void*)&w),
      DataRef(Type_Get<Data>(), (void*)&msg));
  }
}

LTE_TEST(MethodCall_ThisMethodWithWidgetParam_MutatesFields) {
  Script_ClearCache();
  Script script = Script_Load("TestMethodCall");
  LTE_CHECK(script);
  if (!script) return;

  ScriptType type = script->GetType("TestMethodCall");
  LTE_CHECK(type);
  if (!type) return;

  /* A dummy non-null Widget placeholder. Registers hold pointers to the
     variable storage, so a null Reference would make GetLValue() return
     nullptr and take the allocate+Assign path (which dereferences the null
     source). The fixture methods never touch `self`, so any non-null pointer
     is fine. */
  static char dummyWidgetStorage;
  void* widget = &dummyWidgetStorage;

  /* Baseline: DirectSet (bare field write, no this-call) must land. */
  {
    void* instance = Instance(type);
    ScriptFunction fn = type->GetFunction("DirectSet");
    LTE_CHECK(fn);
    if (fn) {
      void* args[] = { instance, widget };
      fn->Call(nullptr, args);
      LTE_CHECK_EQ(GetInt(type, instance, "n"), 42);
    }
    type->type->Deallocate(instance);
  }

  /* this.DoThing self -> DoThing body writes n = n + 1. */
  {
    void* instance = Instance(type);
    ScriptFunction fn = type->GetFunction("Caller");
    LTE_CHECK(fn);
    if (fn) {
      void* args[] = { instance, widget };
      fn->Call(nullptr, args);
      LTE_CHECK_EQ(GetInt(type, instance, "n"), 1);
    }
    type->type->Deallocate(instance);
  }

  /* this.DoDelete self with a non-empty `name` -> DoDelete body writes
     n = 999 (mirrors the SaveGameManager DoDelete guard + action shape). */
  {
    void* instance = Instance(type);
    ScriptFunction fn = type->GetFunction("DeleteCaller");
    LTE_CHECK(fn);
    if (fn) {
      SetString(type, instance, "name", "slotA");
      void* args[] = { instance, widget };
      fn->Call(nullptr, args);
      LTE_CHECK_EQ(GetInt(type, instance, "n"), 999);
    }
    type->type->Deallocate(instance);
  }

  /* Guard: empty `name` takes the early-return path (n stays 0). */
  {
    void* instance = Instance(type);
    ScriptFunction fn = type->GetFunction("DeleteCaller");
    LTE_CHECK(fn);
    if (fn) {
      void* args[] = { instance, widget };
      fn->Call(nullptr, args);
      LTE_CHECK_EQ(GetInt(type, instance, "n"), 0);
    }
    type->type->Deallocate(instance);
  }

  Script_ClearCache();
}

/* Isolated reproducer: does `this.Method self` dispatch from inside a switch
   branch (the SaveGameManager Receive pattern)? Uses the SwitchProbe fixture
   type so there are no SaveGame bindings / large locals in the way. */
LTE_TEST(MethodCall_SwitchProbe_DeleteBranchCallsDoDelete) {
  Script_ClearCache();
  Script script = Script_Load("TestMethodCall");
  LTE_CHECK(script);
  if (!script) return;

  ScriptType type = script->GetType("SwitchProbe");
  ScriptType msgType = script->GetType("ProbeMsgDelete");
  LTE_CHECK(type);
  LTE_CHECK(msgType);
  if (!type || !msgType) return;

  void* instance = type->type->Allocate();
  void* msgInst = msgType->type->Allocate();
  Data msg(msgType->type, msgInst);
  msgType->type->Deallocate(msgInst);

  Reference<WidgetT> w = new WidgetT;

  ScriptFunction receive = type->GetFunction("Receive");
  LTE_CHECK(receive);
  if (receive) {
    receive->VoidCall(0,
      DataRef(type->type, instance),
      DataRef(Type_Get<Widget>(), (void*)&w),
      DataRef(Type_Get<Data>(), (void*)&msg));
  }

  std::printf("  [diag] switch-probe delete: n=%d status='%s'\n",
    GetInt(type, instance, "n"), GetString(type, instance, "status").c_str());
  LTE_CHECK_EQ(GetInt(type, instance, "n"), 777);

  type->type->Deallocate(instance);
  Script_ClearCache();
}

/* Isolated reproducer: does the select branch (field sets, no Rebuild) work
   through the switch? */
LTE_TEST(MethodCall_SwitchProbe_SelectBranchLoadsFields) {
  Script_ClearCache();
  Script script = Script_Load("TestMethodCall");
  LTE_CHECK(script);
  if (!script) return;

  ScriptType type = script->GetType("SwitchProbe");
  ScriptType msgType = script->GetType("ProbeMsgSelect");
  LTE_CHECK(type);
  LTE_CHECK(msgType);
  if (!type || !msgType) return;

  void* instance = type->type->Allocate();
  void* msgInst = msgType->type->Allocate();
  SetString(msgType, msgInst, "slotName", "slotA");
  Data msg(msgType->type, msgInst);
  msgType->type->Deallocate(msgInst);

  Reference<WidgetT> w = new WidgetT;

  ScriptFunction receive = type->GetFunction("Receive");
  LTE_CHECK(receive);
  if (receive) {
    receive->VoidCall(0,
      DataRef(type->type, instance),
      DataRef(Type_Get<Widget>(), (void*)&w),
      DataRef(Type_Get<Data>(), (void*)&msg));
  }

  std::printf("  [diag] switch-probe select: editMode=%d slotName='%s' status='%s'\n",
    (int)GetBool(type, instance, "editMode"),
    GetString(type, instance, "slotName").c_str(),
    GetString(type, instance, "status").c_str());
  LTE_CHECK(GetBool(type, instance, "editMode"));
  LTE_CHECK_EQ(GetString(type, instance, "slotName"), String("slotA"));
  LTE_CHECK_EQ(GetString(type, instance, "status"), String("selected"));

  type->type->Deallocate(instance);
  Script_ClearCache();
}

/* End-to-end through the REAL Widget/SaveGameManager script: send a
   MessageSaveDelete through Receive (WidgetCustom::Receive pattern) and verify
   the C++ side actually deletes the slot file and the instance fields are
   cleared. Uses a scratch copy of a real quicksave so the user's saves are
   untouched. */
LTE_TEST(MethodCall_Manager_ReceiveDelete_DeletesSlot) {
  Script_ClearCache();

  /* This test drives the real save dir, so override whatever directory the
     TestSaveGameJSON suite redirected the layer to. */
  SaveGame_SetSavesDir("./cache/saves/");

  std::string const saveDir = "./cache/saves/";
  std::string const scratch = saveDir + "zz-headless-delete-test.json";
  std::string const srcFile = saveDir + "quick-20260813-004717.json";
  if (FileExists(srcFile))
    CopyFile(srcFile, scratch);

  if (FileExists(scratch)) {
    Script script = Script_Load("Widget/SaveGameManager");
    LTE_CHECK(script);
    if (script) {
      ScriptType type = script->GetType("SaveGameManager");
      ScriptType msgType = script->GetType("MessageSaveDelete");
      LTE_CHECK(type);
      LTE_CHECK(msgType);
      if (type && msgType) {
        std::printf("  [diag] manager funcs: DoDelete=%s DoSave=%s Receive=%s\n",
          type->GetFunction("DoDelete") ? "found" : "MISSING",
          type->GetFunction("DoSave") ? "found" : "MISSING",
          type->GetFunction("Receive") ? "found" : "MISSING");
        void* instance = type->type->Allocate();
        SetString(type, instance, "slotName", "zz-headless-delete-test");

        void* msgInst = msgType->type->Allocate();
        Data msg(msgType->type, msgInst);
        msgType->type->Deallocate(msgInst);

        std::printf("  [diag] msg.type->name = %s, msgType->type->name = %s, "
          "msgType->name = %s, manager type->type->name = %s\n",
          msg.type ? msg.type->name.c_str() : "(null)",
          msgType->type ? msgType->type->name.c_str() : "(null)",
          msgType->name.c_str(),
          type->type ? type->type->name.c_str() : "(null)");

        Reference<WidgetT> w = new WidgetT;
        Deliver(type, instance, w, msg);

        std::printf("  [diag] after deliver: slotName='%s' status='%s' fileExists=%d\n",
          GetString(type, instance, "slotName").c_str(),
          GetString(type, instance, "status").c_str(),
          (int)FileExists(scratch));

        LTE_CHECK(!FileExists(scratch));
        LTE_CHECK_EQ(GetString(type, instance, "slotName"), String(""));
        LTE_CHECK_EQ(GetString(type, instance, "status"),
          String("Deleted 'zz-headless-delete-test'."));

        type->type->Deallocate(instance);
      }
    }
  }

  Script_ClearCache();
}

/* End-to-end: MessageSaveSelect through Receive must enter edit mode, adopt
   the message's slotName, and load the slot's metadata (saveName here is empty
   for a quicksave-derived scratch slot, but the selection fields must land). */
LTE_TEST(MethodCall_Manager_ReceiveSelect_LoadsFields) {
  Script_ClearCache();

  /* Same as the delete test: this drives the real save dir. */
  SaveGame_SetSavesDir("./cache/saves/");

  std::string const saveDir = "./cache/saves/";
  std::string const scratch = saveDir + "zz-headless-select-test.json";
  std::string const srcFile = saveDir + "quick-20260813-004717.json";
  if (FileExists(srcFile))
    CopyFile(srcFile, scratch);

  if (FileExists(scratch)) {
    Script script = Script_Load("Widget/SaveGameManager");
    LTE_CHECK(script);
    if (script) {
      ScriptType type = script->GetType("SaveGameManager");
      ScriptType msgType = script->GetType("MessageSaveSelect");
      LTE_CHECK(type);
      LTE_CHECK(msgType);
      if (type && msgType) {
        void* instance = type->type->Allocate();

        void* msgInst = msgType->type->Allocate();
        SetString(msgType, msgInst, "slotName", "zz-headless-select-test");
        Data msg(msgType->type, msgInst);
        msgType->type->Deallocate(msgInst);

        Reference<WidgetT> w = new WidgetT;
        Deliver(type, instance, w, msg);

        LTE_CHECK(GetBool(type, instance, "editMode"));
        LTE_CHECK_EQ(GetString(type, instance, "slotName"),
          String("zz-headless-select-test"));
        LTE_CHECK_EQ(GetString(type, instance, "status").length() > 0, true);

        type->type->Deallocate(instance);
      }
    }
  }

  /* Leave no scratch file behind in the user's saves dir. */
  if (FileExists(scratch))
    std::remove(scratch.c_str());

  Script_ClearCache();
}
