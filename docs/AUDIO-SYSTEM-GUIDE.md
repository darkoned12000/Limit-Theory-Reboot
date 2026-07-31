# Audio System Deep-Dive — Sound, Music, & Ambient Audio

**Date:** 2026-07-30  
**Purpose:** Complete guide to wiring sound effects, music, and ambient audio in Limit Theory

---

## Executive Summary

The engine has a **fully functional audio system** (SFML Audio backend) with:
- ✅ 2D sounds (UI clicks, explosions)
- ✅ 3D positional audio (ship engines, weapon fire)
- ✅ Looping ambient music
- ✅ Volume/pitch/pan control
- ✅ 300+ sound files already included (`resource/sound/`)

**Current Usage:** Minimal. Most apps have NO background music, minimal sound effects.

**Opportunity:** Wire in ambient music, engine hums, combat sounds, UI feedback → Massive immersion boost.

---

## Part 1: How the Audio System Works

### Architecture Overview

```
SoundEngine (abstract interface)
  ├─ SoundEngineSFMLImpl (SFML Audio backend)
  └─ SoundEngineNullImpl (silent fallback)

SoundT (abstract sound instance)
  ├─ SoundSFMLImpl (SFML sf::Sound wrapper)
  └─ SoundNullImpl (no-op)
```

**Files:**
- `src/liblt/Module/SoundEngine.cpp` — Base interface
- `src/liblt/Module/SoundEngine/SFML.cpp` — SFML implementation (used)
- `src/liblt/Module/SoundEngine/Null.cpp` — Silent stub
- `src/liblt/Module/ScriptAPI/SoundEngine.cpp` — LTSL bindings

**Current Backend:** SFML 3.1.0 with miniaudio (OpenAL removed in SFML 3.x).

---

## Part 2: LTSL Sound API (Script-Side)

### 2D Sounds (UI, Explosions, Non-Positional)

**Play Once:**

```lts
# Plays sound at fixed volume, no looping
Sound_Play "ui/hover.wav" 0.5  # 50% volume
```

**Play Looped (Background Music):**

```lts
# Loops forever at 5% volume
Sound_PlayLooped "system/ambiance/089.wav" 0.05
```

**3D Positional Sound (Attached to Object):**

```lts
# Ship engine sound (follows ship position)
var ship (player.GetPiloting)
Sound_Play3D "ship/engine_hum.ogg" ship (Vec3 0) 1.0  # 100% volume, no offset
```

**API Reference:**

| Function | Arguments | Purpose |
|----------|-----------|---------|
| `Sound_Play` | `path:string, volume:float` | Play 2D sound once |
| `Sound_PlayLooped` | `path:string, volume:float` | Loop 2D sound forever |
| `Sound_Play3D` | `path:string, carrier:Object, offset:Vec3, volume:float` | Play 3D positional sound attached to object |
| `Sound_StopAll` | (none) | Stop all playing sounds |

**Volume Range:** `0.0` (silent) to `1.0` (max). Most UI sounds use `0.008` - `0.2`.

---

## Part 3: Available Sound Assets

**Catalog (300+ files in `resource/sound/`):**

| Category | Count | Examples | Use Cases |
|----------|-------|----------|-----------|
| **UI** | ~30 | `hover.wav`, `click_off.wav`, `confirm.ogg` | Button clicks, menu navigation |
| **Weapons** | ~50 | `laser.ogg`, `missile.wav`, `turret.ogg` | Projectile fire, beam weapons |
| **Explosions** | ~20 | `small.ogg`, `large.ogg`, `debris.wav` | Ship destruction, asteroid impacts |
| **Ships** | ~40 | `engine_hum.ogg`, `thruster.wav`, `damage.ogg` | Engine loops, acceleration bursts |
| **Stations** | ~15 | `docking.ogg`, `airlock.wav`, `alarm.ogg` | Docking sequences, station ambiance |
| **System Ambiance** | ~60 | `089.wav`, `nebula.ogg`, `starfield.ogg` | Background drones, space atmosphere |
| **Warp/Jump** | ~10 | `warpnode.ogg`, `jumpdrive.wav`, `exit.ogg` | Warp gate activation, FTL travel |
| **Shields** | ~15 | `activate.ogg`, `hit.wav`, `depleted.ogg` | Shield impacts, energy fields |
| **Scanner** | ~8 | `scanneropen.wav`, `beep.ogg`, `sweep.wav` | Radar pings, target acquisition |

**File Formats:** `.ogg` (Vorbis), `.wav` (PCM), `.flac` (lossless)

**All sounds are royalty-free** (Josh's original assets + public domain).

---

## Part 4: Wiring Background Music

### Problem: Most Apps Have NO Music

**Current State:**
- `war.lts` — No music
- `dogfight.lts` — No music
- `ltheory-main.lts` — Has music! (`system/ambiance/089.wav` at 5% volume)

**Solution: Add Ambient Music to All Apps**

---

### Step 4.1: Add Music to `war.lts` (2 minutes)

**File:** `resource/script/App/war.lts`

**Add in `Initialize()` function (after camera creation):**

```lts
function Void Initialize ()
  # ... existing camera/UI setup ...
  
  # === BACKGROUND MUSIC (ADD THIS) ===
  Sound_PlayLooped "system/ambiance/089.wav" 0.05  # 5% volume
  # OR for more intense combat music:
  # Sound_PlayLooped "system/ambiance/combat_01.ogg" 0.08
```

**Verification:** Run `war.lts` → Hear low-frequency ambient drone.

---

### Step 4.2: Dynamic Music System (Intensity-Based)

**Goal:** Music changes based on gameplay state (exploration → combat → victory).

**Example: Combat Intensity Ramp**

```lts
type MusicManager
  Float combatIntensity 0.0    # 0.0 = peaceful, 1.0 = intense
  Sound ambientTrack            # Low-intensity loop
  Sound combatTrack             # High-intensity loop
  
  function Void Initialize ()
    # Start with ambient
    ambientTrack = Sound_PlayLooped "system/ambiance/peaceful.ogg" 0.05
    
    # Prepare combat track (paused)
    combatTrack = Sound_PlayLooped "system/ambiance/combat.ogg" 0.0
  
  function Void Update (Float dt)
    # Measure nearby enemies
    var enemyCount (CountEnemiesInRange player 50000.0)
    var targetIntensity (Min (enemyCount / 10.0) 1.0)  # 0-10 enemies = 0.0-1.0
    
    # Smooth transition (0.5 second blend)
    combatIntensity = Lerp combatIntensity targetIntensity (dt * 2.0)
    
    # Cross-fade tracks
    ambientTrack.SetVolume (0.05 * (1.0 - combatIntensity))
    combatTrack.SetVolume (0.08 * combatIntensity)
```

**Verification:** Spawn 10 enemy ships → Music intensifies. Destroy them → Music fades back to ambient.

---

## Part 5: 3D Positional Audio

### Current Usage (Minimal)

**Examples Found in Code:**
- `Object/Ship.lts` — Engine hum when thrusting
- `Object/WarpNode.lts` — Warp gate activation sound

**How It Works:**

1. Sound is attached to an **Object** (carrier)
2. Engine updates sound position every frame based on carrier's transform
3. SFML calculates volume/panning based on camera distance

---

### Step 5.1: Add Engine Sound to Player Ship

**File:** `resource/script/App/war.lts`

**Add in `Initialize()` after player ship creation:**

```lts
# Create player ship
var ship shipType.Instantiate
# ... existing ship setup ...

# === ENGINE SOUND (ADD THIS) ===
var engineSound (Sound_Play3D "ship/engine_hum.ogg" ship (Vec3 0 0 -5) 0.3)
engineSound.SetLooped true  # Loop forever

# Store reference so we can control it
player.SetEngineSound engineSound
```

**In `Update()` function:**

```lts
function Void Update ()
  # ... existing update code ...
  
  # === DYNAMIC ENGINE PITCH (ADD THIS) ===
  var ship (player.GetPiloting)
  if ship
    var velocity (ship.GetVelocity.Length)
    var maxSpeed 1000.0
    var speedRatio (Min (velocity / maxSpeed) 1.0)
    
    # Pitch: 0.8 (idle) to 1.5 (max speed)
    var enginePitch (0.8 + 0.7 * speedRatio)
    player.GetEngineSound.SetPitch enginePitch
```

**Result:** Engine hum gets higher-pitched as you accelerate.

---

### Step 5.2: Weapon Fire Sounds

**File:** `resource/script/Object/Ship.lts` (or weapon script)

**Add in weapon fire logic:**

```lts
function Void FireWeapon (Object weapon)
  # ... existing projectile spawn code ...
  
  # === WEAPON SOUND (ADD THIS) ===
  if weapon.GetType == "Beam"
    Sound_Play3D "weapon/laser.ogg" self (Vec3 0) 0.5
  else if weapon.GetType == "Projectile"
    Sound_Play3D "weapon/missile.wav" self (Vec3 0) 0.4
  else if weapon.GetType == "Turret"
    Sound_Play3D "weapon/turret.ogg" self (Vec3 0) 0.6
```

**Result:** Weapon fire plays from ship's position, fades with distance.

---

### Step 5.3: Explosion Sounds

**File:** `resource/script/Object/Ship.lts` (in destruction logic)

**Add in `OnDestroy()` or explosion handler:**

```lts
function Void OnDestroy ()
  # ... existing explosion particle system ...
  
  # === EXPLOSION SOUND (ADD THIS) ===
  var size (self.GetRadius)
  if size < 50.0
    Sound_Play3D "explosion/small.ogg" self (Vec3 0) 0.8
  else if size < 200.0
    Sound_Play3D "explosion/medium.ogg" self (Vec3 0) 1.0
  else
    Sound_Play3D "explosion/large.ogg" self (Vec3 0) 1.0
```

**Result:** Explosions scale with ship size, audible from distance.

---

## Part 6: UI Sound Feedback

### Current Usage (Good!)

UI sounds are **already wired** in most widgets:
- `Widget/Button.lts` — Hover sound (`ui/hover.wav` at 0.8% volume)
- `Widget/Slider.lts` — Click sound when dragging
- `Widget/FocusWindow.lts` — Window open sound
- `Widget/Scanner.lts` — Scanner beep

**These work correctly.** No changes needed.

---

### Optional: Add More UI Feedback

**Example: Confirmation Sound for Critical Actions**

```lts
# In DevPanel's "REGENERATE SYSTEM" button handler:
function Void Receive (Widget self Data data)
  if (data.CastType (Type_Get MessageRegenerate))
    Sound_Play "ui/confirm.ogg" 0.2  # ADD THIS
    RegenerateSystem
    self.Rebuild
```

**Example: Error Sound for Invalid Actions**

```lts
# In market purchase logic:
if player.GetCredits < price
  Sound_Play "ui/error.ogg" 0.3  # ADD THIS
  Print "Insufficient credits!"
  return
```

---

## Part 7: Ambient Environment Sounds

### Goal: Make Space Feel Alive

**Technique:** Layer multiple looping sounds at low volume.

**Example: Nebula Ambiance**

```lts
# In ltheory-main.lts Initialize():
# Spawn nebula (already done)
var nebula (Object_Nebula seed)
nebula.SetPos (Vec3 0)

# === NEBULA AMBIANCE (ADD THIS) ===
Sound_Play3D "system/nebula.ogg" nebula (Vec3 0) 0.03  # Very quiet
```

**Example: Station Interior Hum**

```lts
# When docking at station:
function Void OnDock (Object station)
  # ... existing docking logic ...
  
  # === STATION AMBIANCE (ADD THIS) ===
  Sound_PlayLooped "station/interior_hum.ogg" 0.04
  Sound_PlayLooped "station/machinery.ogg" 0.02
```

**Example: Asteroid Field Creaks**

```lts
# In SystemPopulate.lts, after spawning 1000 asteroids:
# Add a few random creaking sounds scattered through the belt
var rng (RNG_MTG seed)
for i 0 10  # 10 ambient sound sources
  var pos (rng.GetUniform beltInner beltOuter) * rng.Direction
  var dummyObject (Object_Empty)
  dummyObject.SetPos pos
  Sound_Play3D "asteroid/creak.ogg" dummyObject (Vec3 0) 0.01
  dummyObject.SetLoopedSound true
```

---

## Part 8: Performance Considerations

### Sound Limit

**SFML Limit:** ~256 simultaneous sounds (hardware-dependent).

**Current Usage:** ~20 sounds max in war.lts (low).

**Best Practices:**
- Use **sound pooling** for frequent sounds (weapon fire, explosions)
- Stop distant 3D sounds (>100K units from camera)
- Prioritize: Player sounds > Nearby sounds > Distant sounds

---

### Sound Pooling Example

```lts
# Create a pool of 10 laser sounds (reuse instead of spawning)
type SoundPool
  Vector<Sound> laserSounds
  Int nextIndex 0
  
  function Void Initialize ()
    for i 0 10
      var sound (Sound_Create "weapon/laser.ogg")
      laserSounds.Append sound
  
  function Void PlayLaser (Object ship)
    var sound (laserSounds.Get nextIndex)
    sound.SetPosition (ship.GetPos)
    sound.SetVolume 0.5
    sound.Play
    nextIndex = (nextIndex + 1) % 10  # Cycle through pool
```

---

## Part 9: Music Track Recommendations

### Exploration (Low Intensity)

**Recommended Tracks:**
- `system/ambiance/089.wav` — Deep space drone (already used)
- `system/starfield.ogg` — Twinkling starfield ambiance
- `system/peaceful.ogg` — Calm exploration theme

**Volume:** 3-5% (very subtle)

---

### Combat (High Intensity)

**Recommended Tracks:**
- `system/combat_01.ogg` — Fast-paced percussion
- `system/combat_02.ogg` — Synth arpeggios, urgent
- `system/danger.ogg` — Tense low-frequency pulse

**Volume:** 6-10% (more prominent)

---

### Docking/Station (Mechanical)

**Recommended Tracks:**
- `station/interior_hum.ogg` — Low rumble
- `station/machinery.ogg` — Rhythmic clanking
- `station/market.ogg` — Crowd murmur

**Volume:** 2-4% (background)

---

## Part 10: Audio Roadmap

### Phase 1 (1 day): Wire Essential Sounds

**Tasks:**
1. Add background music to `war.lts` (`system/ambiance/089.wav`)
2. Add engine hum to player ship (3D positional)
3. Add weapon fire sounds (laser, missile, turret)
4. Add explosion sounds (small, medium, large)

**Verification:** Play war.lts for 5 minutes → Hear music, engines, combat sounds.

---

### Phase 2 (2 days): Dynamic Music System

**Tasks:**
1. Implement `MusicManager` type (exploration → combat intensity)
2. Cross-fade between ambient/combat tracks based on enemy count
3. Add victory stinger (short jingle when all enemies defeated)

**Verification:** Engage combat → Music intensifies. Win → Victory sound plays.

---

### Phase 3 (1 day): Environmental Audio

**Tasks:**
1. Add nebula ambiance (3D positional, very quiet)
2. Add station interior sounds when docked
3. Add asteroid field creaks (scattered ambient sources)

**Verification:** Fly through nebula → Hear subtle whoosh. Dock → Hear machinery.

---

### Phase 4 (2 days): Advanced Features

**Tasks:**
1. Sound pooling for weapon fire (10-sound circular buffer)
2. Distance-based sound culling (stop sounds >100K units away)
3. Doppler shift for fast-moving objects (pitch based on velocity)
4. Echo/reverb inside stations (requires SFML effects API)

**Verification:** Fire 100 laser shots → No lag (sound pool working). Fly past ship at 500m/s → Hear pitch shift.

---

## Part 11: Common Mistakes & Fixes

### Mistake 1: Sounds Too Loud

**Problem:** `Sound_Play "explosion.ogg" 1.0` → Ear-blasting.

**Fix:** Most sounds should be **0.05 - 0.5** volume. UI sounds: **0.008 - 0.2**.

---

### Mistake 2: No 3D Attenuation

**Problem:** Explosion 100km away sounds same volume as nearby.

**Fix:** Use `Sound_Play3D` with carrier object. SFML auto-attenuates based on distance.

---

### Mistake 3: Looped Music Restarts Every Frame

**Problem:**
```lts
function Void Update ()
  Sound_PlayLooped "music.ogg" 0.05  # WRONG! Restarts every frame
```

**Fix:** Call `Sound_PlayLooped` **once** in `Initialize()`, not in `Update()`.

---

### Mistake 4: Sounds Leak After Object Deletion

**Problem:** Ship destroyed, but engine sound keeps playing.

**Fix:** Store sound reference, call `.Stop()` in `OnDestroy()`:

```lts
function Void OnDestroy ()
  if engineSound
    engineSound.Stop
  # ... rest of destruction logic
```

---

## Part 12: Quick Wins (5-Minute Audio Upgrade)

**Add to `war.lts` Initialize():**

```lts
# Background music
Sound_PlayLooped "system/ambiance/089.wav" 0.05

# Player engine hum
var engineSound (Sound_Play3D "ship/engine_hum.ogg" ship (Vec3 0 0 -5) 0.3)
engineSound.SetLooped true
```

**Add to weapon fire logic (wherever projectiles spawn):**

```lts
Sound_Play3D "weapon/laser.ogg" ship (Vec3 0) 0.4
```

**Add to ship destruction (OnDestroy or similar):**

```lts
Sound_Play3D "explosion/large.ogg" self (Vec3 0) 0.8
```

**Restart war.lts → Instant audio feedback!**

---

## Part 13: Future: Procedural Audio (Advanced)

**Goal:** Generate sounds at runtime (engine hum pitch based on ship specs, explosion timbre based on size).

**Technique:** Use SFML's `sf::SoundBuffer` to write PCM samples directly.

**Example: Procedural Laser Chirp**

```cpp
// Generate 0.5-second laser sound (sweep from 1kHz to 500Hz)
sf::SoundBuffer buffer;
std::vector<sf::Int16> samples(22050);  // 0.5s at 44.1kHz
for (size_t i = 0; i < samples.size(); ++i) {
  float t = (float)i / 22050.0f;
  float freq = 1000.0f - 500.0f * t;  // Sweep down
  float phase = 2.0f * M_PI * freq * t;
  samples[i] = (sf::Int16)(32767.0f * sin(phase) * (1.0f - t));  // Fade out
}
buffer.loadFromSamples(&samples[0], samples.size(), 1, 44100);
```

**Time Estimate:** 1-2 weeks (requires DSP knowledge).

---

## Summary

**What You Have:**
- ✅ Fully functional SFML 3.1.0 audio system
- ✅ 300+ royalty-free sound assets
- ✅ 2D/3D positional audio support
- ✅ UI sounds already wired (button clicks, hovers)

**What's Missing:**
- ❌ Background music in most apps (5-minute fix)
- ❌ Engine/weapon/explosion sounds (1-day task)
- ❌ Dynamic music system (2-day feature)
- ❌ Environmental ambiance (1-day polish)

**Impact:** Audio adds **massive immersion** for minimal effort. Start with Quick Wins (5 minutes) → Hear the difference immediately.

**Next Step:** Copy the "Quick Wins" code into `war.lts`, restart, and enjoy your new soundscape! 🎵
