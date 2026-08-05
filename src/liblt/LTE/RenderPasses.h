#ifndef LTE_RenderPasses_h__
#define LTE_RenderPasses_h__

#include "RenderPass.h"
#include "String.h"

LT_API RenderPass RenderPass_Aberration(
  float const& strength);

LT_API RenderPass RenderPass_Composite(
  Vector<RenderPass> const& passes);

LT_API RenderPass RenderPass_Bloom(
  int const& radius, float const& variance);

LT_API RenderPass RenderPass_BloomLight(
  int const& radius);

LT_API RenderPass RenderPass_CustomFilter(
  Data const& data);

LT_API RenderPass RenderPass_MotionBlur();

LT_API RenderPass RenderPass_PostFilter(
  String const& shaderPath);

LT_API RenderPass RenderPass_RadialBlur(
  V2 const& center, float const& radius, float const& strength, float const& falloff);

LT_API RenderPass RenderPass_Tonemap();

#endif
