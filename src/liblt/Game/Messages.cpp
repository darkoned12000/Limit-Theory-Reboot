#include "Messages.h"
#include "LTE/FunctionBind.h"

static Function const MessageLink_Registration = Function_Bind(
  "MessageLink",
  "",
  [](Object const& object) -> MessageLink
  {
  return MessageLink(object);
  },
  "object");

static Function const MessageBoost_Registration = Function_Bind(
  "MessageBoost",
  "Use Boost Capacitor",
  []() -> MessageBoost
  {
  return MessageBoost();
  });

static Function const MessageCruise_Registration = Function_Bind(
  "MessageCruise",
  "Engage Cruise",
  []() -> MessageCruise
  {
  return MessageCruise();
  });

static Function const MessageFire_Registration = Function_Bind(
  "MessageFire",
  "Fire",
  []() -> MessageFire
  {
  return MessageFire();
  });

static Function const MessageReload_Registration = Function_Bind(
  "MessageReload",
  "Reload",
  []() -> MessageReload
  {
  return MessageReload();
  });

static Function const MessageStartUsing_Registration = Function_Bind(
  "MessageStartUsing",
  "",
  [](Object const& object) -> MessageStartUsing
  {
  return MessageStartUsing(object, nullptr);
  },
  "object");

static Function const MessageStopUsing_Registration = Function_Bind(
  "MessageStopUsing",
  "",
  [](Object const& object) -> MessageStopUsing
  {
  return MessageStopUsing(object);
  },
  "object");

static Function const MessageTargetPosition_Registration = Function_Bind(
  "MessageTargetPosition",
  "Target 'position'",
  [](Position const& position) -> MessageTargetPosition
  {
  return MessageTargetPosition(position);
  },
  "position");

static Function const MessageThrustAngular_Registration = Function_Bind(
  "MessageThrustAngular",
  "Apply an angular thrust of 'magnitude' in the 'dir' local-space direction",
  [](V3 const& dir, float const& magnitude) -> MessageThrustAngular
  {
  return MessageThrustAngular(dir, magnitude);
  },
  "dir", "magnitude");

static Function const MessageThrustLinear_Registration = Function_Bind(
  "MessageThrustLinear",
  "Apply a linear thrust in the 'dir' world-space direction",
  [](V3 const& dir) -> MessageThrustLinear
  {
  return MessageThrustLinear(dir);
  },
  "dir");
