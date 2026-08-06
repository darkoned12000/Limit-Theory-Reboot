#ifndef RenderPasses_h__
#define RenderPasses_h__

#include "Game/Common.h"
#include "LTE/RenderPass.h"
#include "LTE/V3.h"
#include "LTE/V4.h"

LT_API RenderPass RenderPass_Blended();

LT_API RenderPass RenderPass_Camera(
  Camera const& camera);

LT_API RenderPass RenderPass_Clear(
  V4 const& value);

LT_API RenderPass RenderPass_ClearDepth();

LT_API RenderPass RenderPass_DepthPrepass();

LT_API RenderPass RenderPass_DustClouds();

LT_API RenderPass RenderPass_GBuffer();

LT_API RenderPass RenderPass_GlobalLighting();

LT_API RenderPass RenderPass_LocalLighting();

LT_API RenderPass RenderPass_LensFlares();

LT_API RenderPass RenderPass_Particles();

LT_API RenderPass RenderPass_SMAA();

LT_API RenderPass RenderPass_SSAO();

LT_API RenderPass RenderPass_Visibility();

LT_API RenderPass RenderPass_HiZOcclusion();

#endif
