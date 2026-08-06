#include "LTE/Function.h"
#include "LTE/FunctionBind.h"
#include "LTE/RNG.h"

TypeAlias(Reference<RNGT>, RNG);

static Function const RNG_MTG_Registration = Function_Bind(
  "RNG_MTG",
  "Create a new RNG that uses the Mersenne Twister algorithm, seeded with 'seed'",
  [](int const& seed) -> RNG
  {
  return RNG_MTG((uint)seed);
  },
  "seed");

static Function const RNG_Angle_Registration = Function_Bind(
  "RNG_Angle",
  "Generate a uniform random angle between 0 and 2pi using 'rng'",
  [](RNG const& rng) -> float
  {
  return rng->GetAngle();
  },
  "rng");
static int const RNG_Angle_Alias = Function_Alias("RNG_Angle", "Angle");

static Function const RNG_Direction_Registration = Function_Bind(
  "RNG_Direction",
  "Generate a uniform random (unit-length) vector using 'rng'",
  [](RNG const& rng) -> V3
  {
  return rng->GetDirection();
  },
  "rng");
static int const RNG_Direction_Alias = Function_Alias("RNG_Direction", "Direction");

static Function const RNG_ExpFloat_Registration = Function_Bind(
  "RNG_ExpFloat",
  "Generate an exponential random float (with mean 1) using 'rng'",
  [](RNG const& rng) -> float
  {
  return rng->GetExp();
  },
  "rng");
static int const RNG_ExpFloat_Alias = Function_Alias("RNG_ExpFloat", "Exp");

namespace Priv1 {
  static Function const RNG_Float_Registration = Function_Bind(
  "RNG_Float",
  "Generate a uniform random float between 0 (inclusive) and 1 (exclusive) using 'rng'",
  [](RNG const& rng) -> float
  {
    return rng->GetFloat();
  
  },
  "rng");
}

namespace Priv2 {
  static Function const RNG_Float_Registration = Function_Bind(
  "RNG_Float",
  "Generate a uniform random float between 'lower' (inclusive) and 'upper' (exclusive) using 'rng'",
  [](RNG const& rng, float const& lower, float const& upper) -> float
  {
    return rng->GetFloat(lower, upper);
  
  },
  "rng", "lower", "upper");
} 


static int const RNG_Float_Alias = Function_Alias("RNG_Float", "Float");

namespace Priv1 {
  static Function const RNG_Gaussian_Registration = Function_Bind(
  "RNG_Gaussian",
  "Generate a gaussian random float with mean 0 and variance 1 using 'rng'",
  [](RNG const& rng) -> float
  {
    return rng->GetGaussian();
  
  },
  "rng");
}

namespace Priv2 {
  static Function const RNG_Gaussian_Registration = Function_Bind(
  "RNG_Gaussian",
  "Generate a gaussian random float with mean 0 and variance 'variance' using 'rng'",
  [](RNG const& rng, float const& variance) -> float
  {
    return variance * rng->GetGaussian();
  
  },
  "rng", "variance");
}


static int const RNG_Gaussian_Alias = Function_Alias("RNG_Gaussian", "Gaussian");

namespace Priv1 {
  static Function const RNG_Int_Registration = Function_Bind(
  "RNG_Int",
  "Generate a uniform random int between 'lower' and 'upper' (inclusive) using 'rng'",
  [](RNG const& rng, int const& lower, int const& upper) -> int
  {
    return rng->GetInt(lower, upper);
  
  },
  "rng", "lower", "upper");
}

namespace Priv2 {
  static Function const RNG_Int_Registration = Function_Bind(
  "RNG_Int",
  "Generate a uniform random int between 0 (inclusive) and 'upper' (exclusive) using 'rng'",
  [](RNG const& rng, int const& upper) -> int
  {
    return rng->GetInt(0, upper - 1);
  
  },
  "rng", "upper");
}

namespace Priv3 {
  static Function const RNG_Int_Registration = Function_Bind(
  "RNG_Int",
  "Generate a uniform random int over all possible integer values using 'rng'",
  [](RNG const& rng) -> int
  {
    return rng->GetInt();
  
  },
  "rng");
}


static int const RNG_Int_Alias = Function_Alias("RNG_Int", "Int");

static Function const RNG_Sign_Registration = Function_Bind(
  "RNG_Sign",
  "Generate -1 or 1 with equal probability using 'rng'",
  [](RNG const& rng) -> V3
  {
  return rng->GetFloat() < 0.5f ? -1.0f : 1.0f;
  },
  "rng");
static int const RNG_Sign_Alias = Function_Alias("RNG_Sign", "Sign");

static Function const RNG_Sphere_Registration = Function_Bind(
  "RNG_Sphere",
  "Generate a uniform random vector that lies within the unit sphere",
  [](RNG const& rng) -> V3
  {
  return rng->GetSphere();
  },
  "rng");
static int const RNG_Sphere_Alias = Function_Alias("RNG_Sphere", "Sphere");

static Function const RNG_Vec2_Registration = Function_Bind(
  "RNG_Vec2",
  "Generate a uniform random Vec2 with components between 'lower' (inclusive) and 'upper' (exclusive) using 'rng'",
  [](RNG const& rng, V2 const& lower, V2 const& upper) -> V2
  {
  return rng->GetV2(lower, upper);
  },
  "rng", "lower", "upper");
static int const RNG_Vec2_Alias = Function_Alias("RNG_Vec2", "Vec2");

static Function const RNG_Vec3_Registration = Function_Bind(
  "RNG_Vec3",
  "Generate a uniform random Vec3 with components between 'lower' (inclusive) and 'upper' (exclusive) using 'rng'",
  [](RNG const& rng, V3 const& lower, V3 const& upper) -> V3
  {
  return rng->GetV3(lower, upper);
  },
  "rng", "lower", "upper");
static int const RNG_Vec3_Alias = Function_Alias("RNG_Vec3", "Vec3");
