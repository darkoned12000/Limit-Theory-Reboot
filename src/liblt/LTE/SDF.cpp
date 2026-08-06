#include "SDFs.h"
#include "Bound.h"
#include "Math.h"
#include "Plane.h"
#include "Vector.h"
#include "LTE/FunctionBind.h"

namespace {
  AutoClassDerived(FractalWorley, SDFT,
    float, seed,
    int, octaves,
    float, lac)
    DERIVED_TYPE_EX(FractalWorley)

    FractalWorley() = default;

    float Evaluate(V3 const& p) const override {
      NOT_IMPLEMENTED
      return 1;
    }

    Bound3 GetBound() const override {
      return Bound3(-FLT_MAX, FLT_MAX);
    }

    String GetCode(String const& p) const override {
      return Stringize()
        | "fcnoise(" | p | ", " | seed | ", "
        | octaves | ", " | lac | ")";
    }
  };

  DERIVED_IMPLEMENT(FractalWorley)

  AutoClassDerived(FractalPerlin, SDFT,
    V3, center,
    int, octaves,
    float, lac)
    DERIVED_TYPE_EX(FractalPerlin)

    FractalPerlin() = default;

    float Evaluate(V3 const& p) const override {
      NOT_IMPLEMENTED
      return 1;
    }

    Bound3 GetBound() const override {
      return Bound3(-FLT_MAX, FLT_MAX);
    }

    String GetCode(String const& p) const override {
      return Stringize()
        | "fsnoise(" | p | " + " | center | ", " | octaves | ", " | lac | ")";
    }
  };

  DERIVED_IMPLEMENT(FractalPerlin)
}

SDF SDF_FractalPerlin(V3 const& center, int const& octaves, float const& lac) {
  return new FractalPerlin(center, octaves, lac);
}
static Function const SDF_FractalPerlin_Registration = Function_Bind(
  "SDF_FractalPerlin",
  "None",
  &SDF_FractalPerlin,
  "center", "octaves", "lac");



SDF SDF_FractalWorley(float const& seed, int const& octaves, float const& lac) {
  return new FractalWorley(seed, octaves, lac);
}
static Function const SDF_FractalWorley_Registration = Function_Bind(
  "SDF_FractalWorley",
  "None",
  &SDF_FractalWorley,
  "seed", "octaves", "lac");



V3 SDFT::Gradient(V3 const& point) const {
  const float ds = 1e-5f;
  return V3(
    Evaluate(point + V3(ds, 0, 0)) - Evaluate(point - V3(ds, 0, 0)),
    Evaluate(point + V3(0, ds, 0)) - Evaluate(point - V3(0, ds, 0)),
    Evaluate(point + V3(0, 0, ds)) - Evaluate(point - V3(0, 0, ds))) / V3(ds);
}

SDF SDFT::Add(SDF const& other) {
  return SDF_Add(this, other);
}

SDF SDFT::Multiply(float value) {
  return SDF_Multiply(this, value);
}

SDF SDFT::Repeat(V3 const& frequency, V3 const& spacing) {
  return SDF_Repeat(this, frequency, spacing);
}

SDF SDFT::Scale(V3 const& s) {
  return SDF_Scale(this, s);
}

SDF SDFT::Translate(V3 const& t) {
  return SDF_Translate(this, t);
}
