#include "LTE/Function.h"
#include "LTE/FunctionBind.h"
#include "LTE/Math.h"
#include "Game/Object.h"
#include "Module/SoundEngine.h"

TypeAlias(Reference<SoundT>, Sound);

static Function const Sound_Delete_Registration = Function_Bind(
  "Sound_Delete",
  "Mark 'sound' for deletion",
  [](Sound const& sound)
  {
  sound->Delete();
  },
  "sound");
static int const Sound_Delete_Alias = Function_Alias("Sound_Delete", "Delete");

static Function const Sound_GetDuration_Registration = Function_Bind(
  "Sound_GetDuration",
  "Return the duration of 'sound' in ms",
  [](Sound const& sound) -> float
  {
  return sound->GetDuration();
  },
  "sound");
static int const Sound_GetDuration_Alias = Function_Alias("Sound_GetDuration", "GetDuration");

static Function const Sound_IsFinished_Registration = Function_Bind(
  "Sound_IsFinished",
  "Return whether 'sound' has finished playing",
  [](Sound const& sound) -> bool
  {
  return sound->IsFinished();
  },
  "sound");
static int const Sound_IsFinished_Alias = Function_Alias("Sound_IsFinished", "IsFinished");

static Function const Sound_None_Registration = Function_Bind(
  "Sound_None",
  "Return a null sound",
  []() -> Sound
  {
  return nullptr;
  });

static Function const Sound_Play2D_Registration = Function_Bind(
  "Sound_Play2D",
  "Create and play 'sound' in 2D at 'volume' (0 = silent, 1 = max volume)",
  [](String const& sound, float const& volume) -> Sound
  {
  return Sound_Play2D(sound, volume, false);
  },
  "sound", "volume");
static int const Sound_Play2D_Alias = Function_Alias("Sound_Play2D", "Sound_Play");

static Function const Sound_Play3D_Registration = Function_Bind(
  "Sound_Play3D",
  "Create and play 'sound' in 3D attached to 'object' with 'offset', 'volume', and 'size'",
  [](String const& sound, Object const& object, V3 const& offset, float const& volume, float const& size) -> Sound
  {
  return Sound_Play3D(sound, object, offset, volume, size, false);
  },
  "sound", "object", "offset", "volume", "size");
static int const Sound_Play3D_Alias = Function_Alias("Sound_Play3D", "Sound_Play");

static Function const Sound_Play2DLooped_Registration = Function_Bind(
  "Sound_Play2DLooped",
  "Create and play 'sound' continuously in 2D at 'volume' (0 = silent, 1 = max volume)",
  [](String const& sound, float const& volume) -> Sound
  {
  return Sound_Play2D(sound, volume, true);
  },
  "sound", "volume");
static int const Sound_Play2DLooped_Alias = Function_Alias("Sound_Play2DLooped", "Sound_PlayLooped");

static Function const Sound_Play3DLooped_Registration = Function_Bind(
  "Sound_Play3DLooped",
  "Create and play 'sound' continuously in 3D attached to 'object' with 'offset', 'volume', and 'size'",
  [](String const& sound, Object const& object, V3 const& offset, float const& volume, float const& size) -> Sound
  {
  return Sound_Play3D(sound, object, offset, volume, size, true);
  },
  "sound", "object", "offset", "volume", "size");
static int const Sound_Play3DLooped_Alias = Function_Alias("Sound_Play3DLooped", "Sound_PlayLooped");

static Function const Sound_RandomizePosition_Registration = Function_Bind(
  "Sound_RandomizePosition",
  "Set the play position of 'sound' to a random point within the sound",
  [](Sound const& sound)
  {
  return sound->SetCursor(Rand(0.0f, sound->GetDuration()));
  },
  "sound");
static int const Sound_RandomizePosition_Alias = Function_Alias("Sound_RandomizePosition", "RandomizePosition");

static Function const Sound_SetCursor_Registration = Function_Bind(
  "Sound_SetCursor",
  "Set the play position of 'sound' to 'cursor' in ms",
  [](Sound const& sound, float const& cursor)
  {
  return sound->SetCursor(cursor);
  },
  "sound", "cursor");
static int const Sound_SetCursor_Alias = Function_Alias("Sound_SetCursor", "SetCursor");

static Function const Sound_SetPitch_Registration = Function_Bind(
  "Sound_SetPitch",
  "Set the play rate of 'sound' to 'pitch' (1 = normal speed)",
  [](Sound const& sound, float const& pitch)
  {
  sound->SetPitch(pitch);
  },
  "sound", "pitch");
static int const Sound_SetPitch_Alias = Function_Alias("Sound_SetPitch", "SetPitch");

static Function const Sound_SetVolume_Registration = Function_Bind(
  "Sound_SetVolume",
  "Set the volume of 'sound' to 'volume' (0 = silent, 1 = max volume)",
  [](Sound const& sound, float const& volume)
  {
  sound->SetVolume(volume);
  },
  "sound", "volume");
static int const Sound_SetVolume_Alias = Function_Alias("Sound_SetVolume", "SetVolume");

static Function const Sound_GetMasterVolume_Registration = Function_Bind(
  "Sound_GetMasterVolume",
  "Return the master volume (0.0 = silent, 1.0 = max)",
  []() -> float
  {
  return GetSoundEngine()->GetMasterVolume();
  });

static Function const Sound_SetMasterVolume_Registration = Function_Bind(
  "Sound_SetMasterVolume",
  "Set the master volume (0.0 = silent, 1.0 = max). "
  "All subsequent sound plays are scaled by this factor.",
  [](float const& volume)
  {
  GetSoundEngine()->SetMasterVolume(volume);
  },
  "volume");
