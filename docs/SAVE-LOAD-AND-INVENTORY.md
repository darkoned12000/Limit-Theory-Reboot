# Save System, JSON Schemas, and Inventory Implementation Guide

**Last Updated:** 2026-07-30  
**Purpose:** Complete guide to save/load mechanics, JSON schemas, and exposing the hidden inventory system

---

## Table of Contents

1. [What's Already in the Engine (Hidden Systems)](#part-1-whats-already-in-the-engine)
2. [JSON Schema Design for Save Files](#part-2-json-schema-design)
3. [Serialization Deep-Dive](#part-3-serialization-deep-dive)
4. [Exposing the Inventory System](#part-4-exposing-the-inventory-system)
5. [Mining & Looting Implementation](#part-5-mining--looting-implementation)
6. [Implementation Roadmap](#part-6-implementation-roadmap)

---

## Part 1: What's Already in the Engine

### 🎉 GOOD NEWS: Cargo System Fully Implemented!

**Discovery:** The inventory/cargo system EXISTS in `src/liblt/Component/Cargo.cpp` but is NOT exposed to the player in any app!

**Existing LTSL Functions (Already Working):**

```lts
# Add items to cargo
ship.AddItem item quantity  # Returns true if successful, false if cargo full

# Remove items from cargo
ship.RemoveItem item quantity  # Returns true if successful

# Check cargo contents
var count (ship.GetItemCount item)  # How many of this item?

# Check cargo capacity
var capacity (ship.GetCapacity)  # Total cargo space
var used (ship.GetUsedCapacity)  # Currently used space

# Iterate through all cargo
for it (ship.GetCargo) it.HasMore it.Advance {
  var item (it.GetItem)
  var quantity (it.GetQuantity)
  Log (item.GetName + ": " + quantity)
}
```

**C++ Implementation (Already Exists):**

```cpp
// src/liblt/Component/Cargo.cpp

struct ComponentCargo {
  Map<Item, Quantity> elements;  // Item → count
  Mass capacity;                 // Total cargo capacity
  Mass currentMass;              // Current cargo weight
  
  bool Add(Item const& item, Quantity count, bool force) {
    // Check if we have room
    Mass requiredMass = item->GetMass() * (float)count;
    Mass freeMass = capacity - currentMass;
    if (!force && requiredMass > freeMass)
      return false;  // Cargo full!
    
    // Add item
    elements[item] += count;
    currentMass += requiredMass;
    return true;
  }
};
```

---

### 🎉 GOOD NEWS: Mining System Fully Implemented!

**Discovery:** Asteroid mining EXISTS in `src/liblt/Game/Action/Mine.cpp` but mining tasks aren't assigned in player scripts!

**C++ Implementation (Already Exists):**

```cpp
// src/liblt/Game/Action/Mine.cpp

struct ActionMine {
  Object object;   // Miner (ship)
  Object target;   // Asteroid
  Position point;  // Mining laser hit point
  
  void Execute(UpdateState& state) const override {
    ObjectT* root = object->GetRoot();
    ComponentMineable* mine = target->GetMineable();
    
    // Extract resources (rate depends on angle)
    float rate = Exp(-6.0f * (1.0f - Dot(phase, mine->phase)));
    rate *= state.dt * RandExp();
    rate *= (float)mine->quantity;
    rate *= 0.25f;
    
    Quantity quantity = Min(mine->quantity, (Quantity)rate);
    
    // ✅ THIS ALREADY ADDS TO CARGO!
    root->AddItem(mine->item, quantity);
    mine->quantity -= quantity;
  }
};
```

**Why You Can't Mine Right Now:**
- Player ship never calls `Action_Mine`
- No "mine asteroid" button in HUD
- No targeting system for asteroids
- Mining beam not wired to player input

---

### 🎉 GOOD NEWS: Serialization Infrastructure Exists!

**Discovery:** `src/liblt/LTE/Serializer.cpp` has full reflection-based serialization (but save/load game UI missing).

**C++ Implementation (Already Exists):**

```cpp
// src/liblt/LTE/Serializer.cpp

struct Serializer {
  void Process(void* data, Type const& type);  // Serialize any type
  void ProcessPointer(void* data, Type const& type);  // Handles pointers
  
  // Saves complex types via reflection (FIELDS macro)
  void operator()(void* data, char const* name, Type const& type, void*) {
    if (type->IsPrimitive())
      Write((char*)data, type->size);
    else if (type->IsPointer())
      ProcessPointer(data, type);
    else
      type->Map(data, *this, nullptr);  // Recurse into fields
  }
};
```

**Reflection-Based Serialization:**

```cpp
// Any class with FIELDS macro is auto-serializable:

AutoClass(PlayerData,
  String, name,
  int, credits,
  Position, location,
  Object, ship)
  
  FIELDS {
    MAPFIELD(name)
    MAPFIELD(credits)
    MAPFIELD(location)
    MAPFIELD(ship)
  }
};

// Save:
PlayerData player = { "Captain", 10000, Vec3(0,0,0), ship };
Serializer s(Location_File("save.bin"), SerializeMode::Write);
s.Process(player);

// Load:
PlayerData player;
Serializer s(Location_File("save.bin"), SerializeMode::Read);
s.Process(player);
```

---

## Part 2: JSON Schema Design

### Why JSON for Save Files?

**Pros:**
- ✅ Human-readable (players can inspect/edit saves)
- ✅ Version-control friendly (Git diffs readable)
- ✅ Easy to debug (text editor shows structure)
- ✅ Moddable (players can tweak values)

**Cons:**
- ❌ Larger file size (~3-5x bigger than binary)
- ❌ Slower to parse (~2-3x slower)

**Verdict:** Use JSON. File size/speed don't matter for save files (load once per session).

---

### Save File Schema v1.0

**File:** `saves/quicksave.json`

```json
{
  "saveVersion": "1.0.0",
  "gameVersion": "0.1.0",
  "timestamp": "2026-07-30T15:42:13Z",
  "playtime": 7200,
  "difficulty": "normal",
  
  "player": {
    "name": "Captain Hague",
    "credits": 125000,
    "reputation": {
      "terran_federation": 50,
      "pirate_guild": -30,
      "mining_corporation": 75
    },
    "statistics": {
      "shipsDestroyed": 42,
      "asteroidsMinedTotal": 1230,
      "creditsEarnedTotal": 500000,
      "distanceTraveled": 1500000
    },
    "position": {
      "sectorID": 12,
      "x": 50000.5,
      "y": 1200.3,
      "z": -30000.8
    },
    "ship": {
      "typeID": "fighter_mk2",
      "customName": "The Condor",
      "hull": 180.0,
      "maxHull": 200.0,
      "shield": 90.0,
      "maxShield": 100.0,
      "energy": 150.0,
      "maxEnergy": 200.0,
      "fuel": 800.0,
      "maxFuel": 1000.0,
      
      "cargo": {
        "capacity": 50.0,
        "usedCapacity": 32.5,
        "items": [
          {
            "itemID": "ore_iron",
            "quantity": 150
          },
          {
            "itemID": "ore_gold",
            "quantity": 25
          },
          {
            "itemID": "weapon_missile_mk1",
            "quantity": 8
          }
        ]
      },
      
      "equipment": {
        "weapon_primary": "laser_cannon_mk3",
        "weapon_secondary": "missile_launcher_mk1",
        "shield_generator": "shield_mk2",
        "engine": "thruster_mk3",
        "scanner": "scanner_longrange"
      }
    }
  },
  
  "universe": {
    "seed": 6679999,
    "currentSector": 12,
    "sectors": [
      {
        "id": 0,
        "name": "Sol System",
        "visited": true,
        "discovered": true,
        "objects": [
          {
            "type": "station",
            "id": "station_sol_prime",
            "position": { "x": 100000, "y": 0, "z": 50000 },
            "faction": "terran_federation",
            "market": {
              "ore_iron": {
                "buyPrice": 10,
                "sellPrice": 8,
                "stock": 5000
              },
              "ore_gold": {
                "buyPrice": 150,
                "sellPrice": 120,
                "stock": 200
              }
            }
          }
        ]
      },
      {
        "id": 12,
        "name": "Betelgeuse Sector",
        "visited": true,
        "discovered": true,
        "objects": []
      }
    ],
    
    "warpGates": [
      { "fromSector": 0, "toSector": 1 },
      { "fromSector": 1, "toSector": 2 },
      { "fromSector": 0, "toSector": 12 }
    ]
  },
  
  "missions": {
    "active": [
      {
        "id": "mission_001",
        "type": "cargo_delivery",
        "giver": "station_sol_prime",
        "target": "station_alpha_centauri",
        "cargoItemID": "ore_iron",
        "cargoQuantity": 100,
        "reward": 5000,
        "timeLimit": 3600,
        "status": "in_progress"
      }
    ],
    "completed": [
      {
        "id": "mission_tutorial",
        "completedAt": "2026-07-29T10:30:00Z",
        "reward": 1000
      }
    ]
  },
  
  "discoveredObjects": [
    "station_sol_prime",
    "planet_earth",
    "asteroid_field_12",
    "wormhole_alpha"
  ],
  
  "settings": {
    "difficulty": "normal",
    "autoSaveInterval": 300
  }
}
```

---

### Save File Schema - Minimal Version (v1.0-minimal)

**For Early Development (Phase 1):**

```json
{
  "saveVersion": "1.0.0-minimal",
  "timestamp": "2026-07-30T15:42:13Z",
  
  "player": {
    "credits": 125000,
    "position": { "sectorID": 0, "x": 0, "y": 0, "z": 0 },
    "ship": {
      "hull": 180.0,
      "maxHull": 200.0,
      "cargo": {
        "items": [
          { "itemID": "ore_iron", "quantity": 150 }
        ]
      }
    }
  },
  
  "universe": {
    "seed": 6679999,
    "currentSector": 0
  }
}
```

**Start here, expand later.**

---

## Part 3: Serialization Deep-Dive

### A. Binary Serialization (Existing System)

**Current Engine Approach:**

```cpp
// Save binary format (uses reflection):
void SaveGame(String const& filename) {
  Location saveFile = Location_File(filename);
  Serializer s(saveFile, SerializeMode::Write);
  
  // Serialize player data
  s.Process(playerData);  // Auto-serializes all FIELDS
  
  // Serialize universe state
  s.Process(universe);
  
  // Serializer closes file in destructor
}

// Load binary format:
void LoadGame(String const& filename) {
  Location saveFile = Location_File(filename);
  Serializer s(saveFile, SerializeMode::Read);
  
  s.Process(playerData);
  s.Process(universe);
}
```

**Pros:**
- ✅ Fast (binary I/O)
- ✅ Small files
- ✅ Auto-serializes via reflection (FIELDS macro)

**Cons:**
- ❌ Not human-readable
- ❌ Version changes break saves (binary format rigid)
- ❌ Hard to debug

---

### B. JSON Serialization (Recommended)

**Using nlohmann/json Library:**

```cpp
// src/liblt/Game/SaveGame.cpp (NEW)

#include "json.hpp"
using json = nlohmann::json;

struct SaveData {
  struct Player {
    String name;
    int credits;
    V3D position;
    
    // Cargo items
    Vector< std::pair<String, int> > cargoItems;
  };
  
  String saveVersion;
  String timestamp;
  Player player;
  int universeSeed;
  int currentSector;
};

void SaveGame(String const& saveName) {
  SaveData data;
  
  // Collect player data
  Object playerShip = Player_GetShip();
  data.player.credits = Player_GetCredits();
  data.player.position = playerShip->GetPos();
  
  // Collect cargo
  for (CargoIterator it = playerShip->GetCargo()->elements.begin();
       it != playerShip->GetCargo()->elements.end(); ++it) {
    String itemID = it->first->GetName();  // Or GetID()
    int quantity = it->second;
    data.player.cargoItems.push_back({itemID, quantity});
  }
  
  // Collect universe state
  data.universeSeed = Universe_GetSeed();
  data.currentSector = Universe_GetCurrentSector();
  
  // Convert to JSON
  json j;
  j["saveVersion"] = "1.0.0";
  j["timestamp"] = GetCurrentTimestamp();
  j["player"]["credits"] = data.player.credits;
  j["player"]["position"]["x"] = data.player.position.x;
  j["player"]["position"]["y"] = data.player.position.y;
  j["player"]["position"]["z"] = data.player.position.z;
  
  // Cargo items
  j["player"]["ship"]["cargo"]["items"] = json::array();
  for (auto& [itemID, quantity] : data.player.cargoItems) {
    j["player"]["ship"]["cargo"]["items"].push_back({
      {"itemID", itemID},
      {"quantity", quantity}
    });
  }
  
  j["universe"]["seed"] = data.universeSeed;
  j["universe"]["currentSector"] = data.currentSector;
  
  // Write to file
  std::ofstream file("saves/" + saveName + ".json");
  file << j.dump(2);  // Pretty-print with 2-space indent
}

void LoadGame(String const& saveName) {
  // Read JSON file
  std::ifstream file("saves/" + saveName + ".json");
  json j = json::parse(file);
  
  // Verify version
  String version = j["saveVersion"];
  if (version != "1.0.0") {
    Log_Error("Save file version mismatch!");
    return;
  }
  
  // Restore player data
  int credits = j["player"]["credits"];
  Player_SetCredits(credits);
  
  V3D position;
  position.x = j["player"]["position"]["x"];
  position.y = j["player"]["position"]["y"];
  position.z = j["player"]["position"]["z"];
  
  // Restore cargo
  Object playerShip = Player_GetShip();
  playerShip->GetCargo()->elements.clear();  // Clear existing cargo
  
  for (auto& itemJson : j["player"]["ship"]["cargo"]["items"]) {
    String itemID = itemJson["itemID"];
    int quantity = itemJson["quantity"];
    
    // Reconstruct item from ID (need item database)
    Item item = ItemDatabase_Get(itemID);
    playerShip->AddItem(item, quantity);
  }
  
  // Restore universe state
  int seed = j["universe"]["seed"];
  int sectorID = j["universe"]["currentSector"];
  
  Universe_Initialize(seed);
  Universe_JumpToSector(sectorID);
  
  // Place player at saved position
  playerShip->SetPos(position);
}
```

---

### C. Versioned Save Files (Migration)

**Problem:** Game updates break old saves.

**Solution:** Version-aware loading with migration.

```cpp
void LoadGame(String const& saveName) {
  json j = json::parse(file);
  String version = j["saveVersion"];
  
  // Migrate old saves
  if (version == "1.0.0") {
    j = MigrateSave_1_0_to_1_1(j);
  }
  if (version == "1.1.0") {
    j = MigrateSave_1_1_to_1_2(j);
  }
  
  // Now load current version
  LoadSave_CurrentVersion(j);
}

json MigrateSave_1_0_to_1_1(json const& oldSave) {
  json newSave = oldSave;
  newSave["saveVersion"] = "1.1.0";
  
  // Add new field (wasn't in v1.0)
  newSave["player"]["ship"]["fuel"] = 1000.0;
  newSave["player"]["ship"]["maxFuel"] = 1000.0;
  
  return newSave;
}
```

---

## Part 4: Exposing the Inventory System

### Current Problem

**Player ship HAS cargo, but NO UI to see it!**

Let's fix this with a full inventory panel.

---

### A. Inventory Widget (LTSL)

**File:** `resource/script/Widget/HUD/Inventory.lts` (NEW)

```lts
type InventoryPanel
  Widget widget
  Object player

function Widget InventoryPanel:Create (Object player) {
  var self (Object_Create "InventoryPanel")
  self.player = player
  
  var ship (player.GetPiloting)
  var l (List)
  
  # Title
  l.Append (Widgets:Text "CARGO HOLD" 24 Colors:Primary)
  l.Append (Widgets:Spacer 10)
  
  # Capacity bar
  var capacity (ship.GetCapacity)
  var used (ship.GetUsedCapacity)
  var percentFull (used / capacity * 100.0)
  
  l.Append (Widgets:Row
    (Widgets:Text ("Capacity: " + used + " / " + capacity) 16 Colors:Text)
    (Widgets:ProgressBar percentFull 200 20 Colors:Primary)
  )
  l.Append (Widgets:Spacer 10)
  
  # Item list (header)
  var header (Widgets:Row)
  header.Add (Widgets:Text "ITEM" 14 Colors:Secondary)
  header.Add (Widgets:Spacer 200)
  header.Add (Widgets:Text "QUANTITY" 14 Colors:Secondary)
  header.Add (Widgets:Spacer 50)
  header.Add (Widgets:Text "MASS" 14 Colors:Secondary)
  header.Add (Widgets:Spacer 50)
  header.Add (Widgets:Text "VALUE" 14 Colors:Secondary)
  l.Append header
  l.Append (Widgets:Spacer 5)
  
  # Item rows
  for it (ship.GetCargo) it.HasMore it.Advance {
    var item (it.GetItem)
    var quantity (it.GetQuantity)
    var totalMass (item.GetMass * quantity)
    var totalValue (item.GetValue * quantity)
    
    var row (Widgets:Row)
    row.Add (Widgets:Text item.GetName 14 Colors:Text)
    row.Add (Widgets:Spacer 200)
    row.Add (Widgets:Text quantity 14 Colors:Text)
    row.Add (Widgets:Spacer 50)
    row.Add (Widgets:Text (totalMass + " kg") 14 Colors:Text)
    row.Add (Widgets:Spacer 50)
    row.Add (Widgets:Text (totalValue + " cr") 14 Colors:Highlight)
    
    # Drop button (eject cargo)
    row.Add (Widgets:Button "Drop" (function {
      if (ship.RemoveItem item quantity) {
        Log ("Ejected " + quantity + " " + item.GetName)
        Sound_Play "ui/click.wav" 0.5
      }
    }))
    
    l.Append row
  }
  
  # Total cargo value
  l.Append (Widgets:Spacer 10)
  var totalValue 0
  for it (ship.GetCargo) it.HasMore it.Advance {
    totalValue = totalValue + (it.GetItem.GetValue * it.GetQuantity)
  }
  l.Append (Widgets:Text ("Total Value: " + totalValue + " credits") 18 Colors:Highlight)
  
  self.widget = (Widgets:Column l)
  return self.widget
}
```

---

### B. Add Inventory Hotkey to HUD

**File:** `resource/script/Widget/HUD.lts` (MODIFY)

```lts
# In HUD layer Update(), add hotkey:

function void Update (Float dt) {
  # ... existing code ...
  
  # Press 'I' to toggle inventory
  if (Keyboard_Pressed Key_I) {
    if (inventoryOpen) {
      # Close inventory
      layerHUD.RemoveChild inventoryPanel
      inventoryOpen = false
    } else {
      # Open inventory
      inventoryPanel = (HUD/Inventory:Create player)
      layerHUD.AddChild inventoryPanel
      inventoryOpen = true
    }
  }
}
```

---

## Part 5: Mining & Looting Implementation

### A. Mining: Expose Existing System

**The mining code EXISTS (`Action/Mine.cpp`), we just need to wire it to player input!**

**File:** `resource/script/Widget/HUD.lts` (ADD MINING)

```lts
# In piloting update loop, add mining hotkey:

function void Update (Float dt) {
  var ship (player.GetPiloting)
  
  # ... existing flight controls ...
  
  # Mining: Hold 'M' to mine targeted asteroid
  if (Keyboard_Down Key_M) {
    var target (player.GetTarget)
    
    if (target.IsNotNull && target.GetType == "Asteroid") {
      # Check range (must be within 500m)
      var distance ((target.GetPos - ship.GetPos).Length)
      if (distance < 500) {
        # Fire mining laser at asteroid
        var hitPoint (target.GetPos + (Vec3 0 0 0))  # Surface point
        var action (Action_Mine ship target hitPoint)
        action.Execute
        
        # Visual feedback: mining beam
        DrawMiningBeam ship target
        Sound_Play "weapon/laser.ogg" 0.3
      } else {
        # Too far away
        if (Keyboard_Pressed Key_M) {
          Log "Target out of range (max 500m)"
        }
      }
    } else {
      # No asteroid targeted
      if (Keyboard_Pressed Key_M) {
        Log "No asteroid targeted. Press 'T' to target nearest asteroid."
      }
    }
  }
}

function void DrawMiningBeam (Object ship, Object asteroid) {
  # Visual: green beam from ship to asteroid
  var beamStart (ship.GetPos)
  var beamEnd (asteroid.GetPos)
  var beam (Object_Create "MiningBeam")
  beam.SetMesh (Generator_Beam beamStart beamEnd 2.0)
  beam.SetShader (Shader "identity.jsl" "beam.jsl")
  beam.SetColor (Vec3 0.2 1.0 0.2)  # Green
  beam.SetLifetime 0.1
  root.AddInterior beam
}
```

---

### B. Asteroid Targeting System

**File:** `resource/script/Widget/HUD.lts` (ADD TARGETING)

```lts
# Global variables:
var targetedObject null

# In Update():

# Press 'T' to target nearest asteroid
if (Keyboard_Pressed Key_T) {
  var ship (player.GetPiloting)
  var nearest (FindNearestAsteroid ship)
  
  if (nearest.IsNotNull) {
    targetedObject = nearest
    player.SetTarget nearest
    Log ("Targeted: " + nearest.GetName)
    Sound_Play "ui/target.wav" 0.7
  } else {
    Log "No asteroids in range"
  }
}

# Draw target reticle
if (targetedObject.IsNotNull) {
  DrawTargetReticle targetedObject
}

function Object FindNearestAsteroid (Object ship) {
  var nearestDist 999999.0
  var nearest null
  
  var objects (ship.GetContainer.GetInteriorObjects)
  for i 0 (objects.GetSize) {
    var obj (objects[i])
    if (obj.GetType == "Asteroid") {
      var dist ((obj.GetPos - ship.GetPos).Length)
      if (dist < nearestDist) {
        nearestDist = dist
        nearest = obj
      }
    }
  }
  
  return nearest
}

function void DrawTargetReticle (Object target) {
  # Project 3D target position to 2D screen
  var screenPos (Camera_WorldToScreen target.GetPos)
  
  # Draw bracket corners around target
  var size 30.0
  Renderer_DrawLine (screenPos + (Vec2 -size -size)) (screenPos + (Vec2 -size*0.5 -size)) (Vec3 1 0 0)
  Renderer_DrawLine (screenPos + (Vec2 -size -size)) (screenPos + (Vec2 -size -size*0.5)) (Vec3 1 0 0)
  # ... (repeat for 4 corners)
  
  # Display target info
  var distance ((target.GetPos - player.GetPiloting.GetPos).Length)
  Renderer_DrawText (target.GetName + " (" + distance + "m)") (screenPos + (Vec2 0 40)) Colors:Primary
}
```

---

### C. NPC Loot Drops

**File:** `resource/script/Object/Ship.lts` (ADD LOOT DROP ON DEATH)

```lts
# When NPC ship dies, drop cargo as floating pods

function void Ship:OnDeath () {
  var self this
  
  # Drop all cargo as loot pods
  for it (self.GetCargo) it.HasMore it.Advance {
    var item (it.GetItem)
    var quantity (it.GetQuantity)
    
    # Create floating cargo pod
    var pod (Object_Pod item quantity)
    pod.SetPos (self.GetPos + (Vec3 (Rand -100 100) (Rand -100 100) (Rand -100 100)))
    pod.SetVel (self.GetVel + (Vec3 (Rand -10 10) (Rand -10 10) (Rand -10 10)))
    pod.SetLifetime 300.0  # Despawn after 5 minutes
    
    self.GetContainer.AddInterior pod
  }
  
  # Drop credits as collectible
  if (self.GetCredits > 0) {
    var creditPod (Object_Pod Item_Credits self.GetCredits)
    creditPod.SetPos (self.GetPos)
    self.GetContainer.AddInterior creditPod
  }
  
  Log (self.GetName + " destroyed! Loot dropped.")
}
```

**File:** `resource/script/Object/Pod.lts` (CREATE LOOT POD)

```lts
type LootPod
  Item item
  Int quantity

function Object Object_Pod (Item item Int quantity) {
  var self (Object_Create "LootPod")
  
  # Visual: small glowing cube
  var mesh (Mesh_Cube 5.0)
  self.SetMesh mesh
  self.SetShader (Shader "identity.jsl" "unlit.jsl")
  self.SetColor (Vec3 1.0 0.8 0.2)  # Gold
  
  # Collision: player picks up by flying through
  var trigger (Physics_CreateSphere 10.0)
  self.SetCollidable trigger
  
  # Store item data
  self.item = item
  self.quantity = quantity
  
  # On collision with player ship
  self.SetEventCollide (function (Object other) {
    if (other.GetType == "Ship" && other.IsPlayer) {
      # Add to player cargo
      if (other.AddItem self.item self.quantity) {
        Log ("Picked up: " + self.quantity + " " + self.item.GetName)
        Sound_Play "ui/pickup.wav" 0.8
        self.Delete
      } else {
        Log "Cargo full! Cannot pick up " + self.item.GetName
      }
    }
  })
  
  return self
}
```

---

## Part 6: Implementation Roadmap

### Phase 1: Minimal Save/Load (1-2 weeks)

**Goal:** Save player credits + position, reload into same sector.

**Files to Create:**
1. `src/liblt/Game/SaveGame.cpp` — JSON save/load functions
2. LTSL bindings: `SaveGame_Create`, `SaveGame_Load`
3. `resource/script/Widget/MainMenu.lts` — Add "Save Game" / "Load Game" buttons

**Minimal Schema:**
```json
{
  "saveVersion": "1.0.0-minimal",
  "player": {
    "credits": 125000,
    "position": { "x": 0, "y": 0, "z": 0 }
  },
  "universe": { "seed": 6679999, "currentSector": 0 }
}
```

**Test:**
1. Start game
2. Move ship, earn credits
3. Save game
4. Quit
5. Load save → verify position + credits restored

---

### Phase 2: Cargo Serialization (3-4 days)

**Goal:** Save/load player cargo.

**Add to Schema:**
```json
"player": {
  "ship": {
    "cargo": {
      "items": [
        { "itemID": "ore_iron", "quantity": 150 }
      ]
    }
  }
}
```

**Implementation:**
- Iterate `ship.GetCargo()` during save
- Reconstruct `ship.AddItem()` during load
- Requires item database (maps `itemID` → `Item` object)

**Test:**
1. Mine asteroids (fill cargo)
2. Save game
3. Load save → verify cargo contents match

---

### Phase 3: Inventory UI (3-4 days)

**Goal:** Show player cargo in HUD.

**Files to Create:**
1. `resource/script/Widget/HUD/Inventory.lts`

**Features:**
- Press 'I' to open inventory panel
- List all cargo items (name, quantity, mass, value)
- Show capacity bar (used / total)
- "Drop" button to eject items

**Test:**
1. Mine asteroids
2. Press 'I' → inventory panel opens
3. Verify items listed
4. Click "Drop" → item ejected from cargo

---

### Phase 4: Mining Mechanics (1 week)

**Goal:** Wire existing `Action_Mine` to player input.

**Files to Modify:**
1. `resource/script/Widget/HUD.lts` — Add mining hotkey ('M')
2. `resource/script/Widget/HUD.lts` — Add targeting system ('T')

**Features:**
- Press 'T' to target nearest asteroid
- Draw target reticle around asteroid
- Hold 'M' to mine (green beam)
- Mined ore → added to cargo automatically (already works!)

**Test:**
1. Fly to asteroid field
2. Press 'T' → asteroid targeted (reticle appears)
3. Hold 'M' → mining beam fires, ore added to cargo
4. Press 'I' → inventory shows mined ore

---

### Phase 5: Loot Drops (3-4 days)

**Goal:** NPCs drop cargo on death.

**Files to Create:**
1. `resource/script/Object/Pod.lts` — Floating loot pod
2. Modify `Object/Ship.lts` — Add `OnDeath` loot drop

**Features:**
- NPC ship dies → drops cargo as floating pods
- Player flies through pod → auto-pickup (adds to cargo)
- Pods despawn after 5 minutes

**Test:**
1. Destroy NPC ship
2. Loot pods spawn at death location
3. Fly through pod → "Picked up: 10 Iron Ore" message
4. Press 'I' → inventory shows looted items

---

### Phase 6: Full Save System (2-3 weeks)

**Goal:** Save everything (universe state, stations, missions).

**Add to Schema:**
- Sector states (visited, discovered)
- Station inventories + market prices
- Active missions
- Faction reputations

**Implementation:**
- Serialize entire universe graph
- Save station `Object` references
- Mission progress tracking

---

### Phase 7: Auto-Save (2-3 days)

**Goal:** Save every 5 minutes automatically.

**Implementation:**
```lts
var autoSaveTimer 0.0

function void Update (Float dt) {
  autoSaveTimer = autoSaveTimer + dt
  
  if (autoSaveTimer > 300.0) {  # 5 minutes
    SaveGame_Create "autosave"
    Log "Auto-saved game"
    autoSaveTimer = 0.0
  }
}
```

---

## Summary: What You Need to Do

### 🎉 Good News: Core Systems Exist!

1. ✅ **Cargo system fully implemented** (`Component/Cargo.cpp`)
2. ✅ **Mining system fully implemented** (`Action/Mine.cpp`)
3. ✅ **Serialization infrastructure exists** (`Serializer.cpp`)

### 🚧 What's Missing: UI & Wiring

1. ❌ **Inventory UI** (press 'I' to see cargo)
2. ❌ **Mining hotkey** (press 'M' to mine asteroids)
3. ❌ **Targeting system** (press 'T' to target)
4. ❌ **Save/Load UI** (main menu buttons)
5. ❌ **Loot drops** (NPCs drop cargo on death)

---

## Quick Start: Try Cargo System RIGHT NOW

**You can test the cargo system immediately in any app!**

**File:** `resource/script/App/war.lts` (ADD TEST CODE)

```lts
# In Initialize(), after ship spawned:

# Test cargo system
var ship (player.GetPiloting)
Log ("Ship cargo capacity: " + ship.GetCapacity)

# Add test items
var ironOre (Item_OreType 100)  # Create iron ore item
if (ship.AddItem ironOre 50) {
  Log "Added 50 iron ore to cargo"
}

# Check cargo contents
for it (ship.GetCargo) it.HasMore it.Advance {
  var item (it.GetItem)
  var quantity (it.GetQuantity)
  Log ("Cargo: " + item.GetName + " x" + quantity)
}

# Remove item
if (ship.RemoveItem ironOre 10) {
  Log "Removed 10 iron ore from cargo"
}

Log ("Cargo used: " + ship.GetUsedCapacity + " / " + ship.GetCapacity)
```

**Run:**
```bash
python configure.py run war
```

**Expected Output (in console):**
```
Ship cargo capacity: 100
Added 50 iron ore to cargo
Cargo: Iron Ore x50
Removed 10 iron ore from cargo
Cargo used: 40 / 100
```

**✅ The inventory system WORKS! It just needs UI!**

---

## Next Steps

1. **Read this doc** (you're doing it!)
2. **Test cargo system** (add test code to `war.lts`)
3. **Implement Phase 1** (minimal save/load, 1-2 weeks)
4. **Implement Phase 3** (inventory UI, 3-4 days)
5. **Implement Phase 4** (mining hotkey, 1 week)

---

**Total Timeline: 5-6 weeks for complete save/load + inventory + mining system.**

**You're closer than you think! The engine has everything you need — it just needs UI wiring!** 🚀✨

**Questions? Want to dive deeper into JSON schemas, item databases, or serialization patterns? Let's do it!**
