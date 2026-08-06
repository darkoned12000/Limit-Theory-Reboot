#include "Particles.h"
#include "LTE/FunctionBind.h"

namespace {
  Shader gRadialShader;
  Shader gRadialTexturedShader;

  void StaticInitialize() {
    if (!gRadialShader) {
      gRadialShader =
        Shader_Create("particle.jsl", "particle_radial.jsl");
      gRadialTexturedShader =
        Shader_Create("particle.jsl", "particle_radialtextured.jsl");
    }
  }
}

ShaderInstance Particle_Fire() {
  StaticInitialize();
  static ShaderInstance shader;
  if (!shader) {
    shader = ShaderInstance_Create(gRadialShader);
    (*shader)
      ("baseColor", V3(1))
      ("falloff", 5.0f)
      ("opacity", 0.5f)
      ("fadeIn", 0.5f)
      ("fadeOut", 0.5f)
      (RenderStateSwitch_BlendModeAdditive)
      (RenderStateSwitch_ZWritableOff);
  }
  return shader;
}
static Function const Particle_Fire_Registration = Function_Bind(
  "Particle_Fire",
  "None",
  &Particle_Fire);



ShaderInstance Particle_Firefly() {
  StaticInitialize();
  static ShaderInstance shader;
  if (!shader) {
    shader = ShaderInstance_Create(gRadialShader);
    (*shader)
      ("baseColor", V3(1))
      ("falloff", 5.0f)
      ("opacity", 0.2f)
      ("fadeIn", 0.2f)
      ("fadeOut", 0.2f)
      (RenderStateSwitch_BlendModeAdditive)
      (RenderStateSwitch_ZWritableOff);
  }
  return shader;
}
static Function const Particle_Firefly_Registration = Function_Bind(
  "Particle_Firefly",
  "None",
  &Particle_Firefly);


