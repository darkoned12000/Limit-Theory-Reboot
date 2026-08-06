#ifndef Graphics_Renderables_h__
#define Graphics_Renderables_h__

#include "LTE/Generic.h"
#include "LTE/AutoClass.h"

AutoClass(Renderable_Asteroid_Args,
  uint, seed)
  Renderable_Asteroid_Args() {}
};

LT_API Renderable Renderable_Asteroid(Renderable_Asteroid_Args const& args);
inline Renderable Renderable_Asteroid(
  uint const& seed) {
  return Renderable_Asteroid(Renderable_Asteroid_Args(seed));
}

AutoClass(Renderable_Ice_Args,
  uint, seed)
  Renderable_Ice_Args() {}
};

LT_API Renderable Renderable_Ice(Renderable_Ice_Args const& args);
inline Renderable Renderable_Ice(
  uint const& seed) {
  return Renderable_Ice(Renderable_Ice_Args(seed));
}

AutoClass(Renderable_Imposter_Args,
  Renderable, source)
  Renderable_Imposter_Args() {}
};

LT_API Renderable Renderable_Imposter(Renderable_Imposter_Args const& args);
inline Renderable Renderable_Imposter(
  Renderable const& source) {
  return Renderable_Imposter(Renderable_Imposter_Args(source));
}

AutoClass(Renderable_Starfield_Args,
  uint, seed,
  uint, starCount)
  Renderable_Starfield_Args() {}
};

LT_API Renderable Renderable_Starfield(Renderable_Starfield_Args const& args);
inline Renderable Renderable_Starfield(
  uint const& seed, uint const& starCount) {
  return Renderable_Starfield(Renderable_Starfield_Args(seed, starCount));
}

#endif
