#include "Game/Player.h"

#include "LTE/Function.h"
#include "LTE/FunctionBind.h"

TypeAlias(Reference<PlayerT>, Player);

static void player_to_object_Impl(Player const& src, Object& dest) {
  dest = (Object)src;
}
static int const player_to_object_Registration = Conversion_Bind<&player_to_object_Impl>();

static Function const Player_GetPiloting_Registration = Function_Bind(
  "Player_GetPiloting",
  "Return the object that 'player' is currently piloting'",
  [](Player const& player) -> Object
  {
  return player->piloting;
  },
  "player");
static int const Player_GetPiloting_Alias = Function_Alias("Player_GetPiloting", "GetPiloting");

static Function const Player_Pilot_Registration = Function_Bind(
  "Player_Pilot",
  "Move 'player' into 'object's pilot seat",
  [](Player const& player, Object const& object)
  {
  return player->Pilot(object);
  },
  "player", "object");
static int const Player_Pilot_Alias = Function_Alias("Player_Pilot", "Pilot");
