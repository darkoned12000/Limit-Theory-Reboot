#ifndef Game_Tasks_h__
#define Game_Tasks_h__

#include "Task.h"
#include "Object.h"
#include "Player.h"
#include "LTE/AutoClass.h"


AutoClass(Task_Buy_Args,
  Object, target,
  Item, item,
  Quantity, quantity,
  Task, task)
  Task_Buy_Args() {}
};

LT_API Task Task_Buy(Task_Buy_Args const& args);
inline Task Task_Buy(
  Object const& target, Item const& item, Quantity const& quantity, Task const& task) {
  return Task_Buy(Task_Buy_Args(target, item, quantity, task));
}

LT_API Task Task_Custom(
  Data const& data);

AutoClass(Task_Destroy_Args,
  Object, target)
  Task_Destroy_Args() {}
};

LT_API Task Task_Destroy(Task_Destroy_Args const& args);
inline Task Task_Destroy(
  Object const& target) {
  return Task_Destroy(Task_Destroy_Args(target));
}

AutoClass(Task_Dock_Args,
  Object, target)
  Task_Dock_Args() {}
};

LT_API Task Task_Dock(Task_Dock_Args const& args);
inline Task Task_Dock(
  Object const& target) {
  return Task_Dock(Task_Dock_Args(target));
}

AutoClass(Task_Drill_Args,
  Object, target)
  Task_Drill_Args() {}
};

LT_API Task Task_Drill(Task_Drill_Args const& args);
inline Task Task_Drill(
  Object const& target) {
  return Task_Drill(Task_Drill_Args(target));
}

AutoClass(Task_Goto_Args,
  Object, target,
  Distance, distance)
  Task_Goto_Args() {}
};

LT_API Task Task_Goto(Task_Goto_Args const& args);
inline Task Task_Goto(
  Object const& target, Distance const& distance) {
  return Task_Goto(Task_Goto_Args(target, distance));
}

AutoClass(Task_LOD_Args,
  Object, owner,
  Task, task)
  Task_LOD_Args() {}
};

LT_API Task Task_LOD(Task_LOD_Args const& args);
inline Task Task_LOD(
  Object const& owner, Task const& task) {
  return Task_LOD(Task_LOD_Args(owner, task));
}

AutoClass(Task_Mine_Args,
  Object, zone)
  Task_Mine_Args() {}
};

LT_API Task Task_Mine(Task_Mine_Args const& args);
inline Task Task_Mine(
  Object const& zone) {
  return Task_Mine(Task_Mine_Args(zone));
}

AutoClass(Task_Mint_Args,
  Item, blueprint)
  Task_Mint_Args() {}
};

LT_API Task Task_Mint(Task_Mint_Args const& args);
inline Task Task_Mint(
  Item const& blueprint) {
  return Task_Mint(Task_Mint_Args(blueprint));
}

AutoClass(Task_Patrol_Args,
  Object, zone)
  Task_Patrol_Args() {}
};

LT_API Task Task_Patrol(Task_Patrol_Args const& args);
inline Task Task_Patrol(
  Object const& zone) {
  return Task_Patrol(Task_Patrol_Args(zone));
}

AutoClass(Task_Pirate_Args,
  Object, zone)
  Task_Pirate_Args() {}
};

LT_API Task Task_Pirate(Task_Pirate_Args const& args);
inline Task Task_Pirate(
  Object const& zone) {
  return Task_Pirate(Task_Pirate_Args(zone));
}

AutoClass(Task_Play_Args,
  Player, player)
  Task_Play_Args() {}
};

LT_API Task Task_Play(Task_Play_Args const& args);
inline Task Task_Play(
  Player const& player) {
  return Task_Play(Task_Play_Args(player));
}

AutoClass(Task_Produce_Args,
  Item, chip)
  Task_Produce_Args() {}
};

LT_API Task Task_Produce(Task_Produce_Args const& args);
inline Task Task_Produce(
  Item const& chip) {
  return Task_Produce(Task_Produce_Args(chip));
}

AutoClass(Task_Research_Args,
  Item, blueprint)
  Task_Research_Args() {}
};

LT_API Task Task_Research(Task_Research_Args const& args);
inline Task Task_Research(
  Item const& blueprint) {
  return Task_Research(Task_Research_Args(blueprint));
}

AutoClass(Task_Sell_Args,
  Object, target,
  Item, item,
  Quantity, quantity,
  Task, task)
  Task_Sell_Args() {}
};

LT_API Task Task_Sell(Task_Sell_Args const& args);
inline Task Task_Sell(
  Object const& target, Item const& item, Quantity const& quantity, Task const& task) {
  return Task_Sell(Task_Sell_Args(target, item, quantity, task));
}

AutoClass(Task_Spawn_Args,
  Item, item,
  float, rate)
  Task_Spawn_Args() {}
};

LT_API Task Task_Spawn(Task_Spawn_Args const& args);
inline Task Task_Spawn(
  Item const& item, float const& rate) {
  return Task_Spawn(Task_Spawn_Args(item, rate));
}

AutoClass(Task_Transport_Args,
  Object, source,
  Object, dest,
  Item, item)
  Task_Transport_Args() {}
};

LT_API Task Task_Transport(Task_Transport_Args const& args);
inline Task Task_Transport(
  Object const& source, Object const& dest, Item const& item) {
  return Task_Transport(Task_Transport_Args(source, dest, item));
}

AutoClass(Task_Wait_Args,
  float, duration)
  Task_Wait_Args() {}
};

LT_API Task Task_Wait(Task_Wait_Args const& args);
inline Task Task_Wait(
  float const& duration) {
  return Task_Wait(Task_Wait_Args(duration));
}

LT_API Task Task_Manage(
  Project const& project);

#endif
