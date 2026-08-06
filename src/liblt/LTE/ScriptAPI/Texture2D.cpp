#include "LTE/Function.h"
#include "LTE/FunctionBind.h"
#include "LTE/Renderer.h"
#include "LTE/ShaderInstance.h"
#include "LTE/Texture2D.h"

#include "LTE/Transform.h"
#include "LTE/Viewport.h"

#include "UI/WidgetRenderer.h"

TypeAlias(Reference<Texture2DT>, Texture2D);

static Function const Texture2D_BeginDrawTo_Registration = Function_Bind(
  "Texture2D_BeginDrawTo",
  "Start using 'texture' as the destination for drawing operations",
  [](Texture2D const& texture)
  {
  texture->Bind(0);
  Renderer_SetWorldTransform(Transform_Identity());
  Renderer_SetViewTransform(Transform_Identity());
  Viewport_Push(Viewport_Create(0, V2(texture->GetWidth(), texture->GetHeight()), 1, false));
  Viewport_Get()->LoadMatrix();
  },
  "texture");
static int const Texture2D_BeginDrawTo_Alias = Function_Alias("Texture2D_BeginDrawTo", "BeginDrawTo");

static Function const DrawClear_Registration = Function_Bind(
  "DrawClear",
  "Clear all pixels of the current drawing surface to have 'color' and 'alpha'",
  [](Color const& color, float const& alpha)
  {
  Renderer_PushScissorOff();
  Renderer_Clear(V4(color.x, color.y, color.z, alpha));
  Renderer_PopScissor();
  },
  "color", "alpha");

static Function const Texture2D_Create_Registration = Function_Bind(
  "Texture2D_Create",
  "Create a new 2-dimensional texture",
  [](int const& width, int const& height) -> Texture2D
  {
  return Texture_Create(width, height);
  },
  "width", "height");

static Function const Texture2D_CreateHDR_Registration = Function_Bind(
  "Texture2D_CreateHDR",
  "Create a new 2-dimensional, high-dynamic-range texture",
  [](int const& width, int const& height) -> Texture2D
  {
  return Texture_Create(width, height, GL_TextureFormat::RGBA32F);
  },
  "width", "height");

static Function const Texture2D_EndDrawTo_Registration = Function_Bind(
  "Texture2D_EndDrawTo",
  "Stop using 'texture' as the destination for drawing operations",
  [](Texture2D const& texture)
  {
  WidgetRenderer_Flush();
  Viewport_Pop();
  texture->Unbind();
  },
  "texture");
static int const Texture2D_EndDrawTo_Alias = Function_Alias("Texture2D_EndDrawTo", "EndDrawTo");

static Function const Texture2D_GenerateFromShader_Registration = Function_Bind(
  "Texture2D_GenerateFromShader",
  "Fill 'texture' by executing 'shader'",
  [](Texture2D const& texture, Shader const& shader)
  {
  Texture_Generate(texture, shader);
  },
  "texture", "shader");
static int const Texture2D_GenerateFromShader_Alias = Function_Alias("Texture2D_GenerateFromShader", "GenerateFromShader");

static Function const Texture2D_GenerateFromShaderInstance_Registration = Function_Bind(
  "Texture2D_GenerateFromShaderInstance",
  "Fill 'texture' by executing 'shaderInstance'",
  [](Texture2D const& texture, ShaderInstance const& shaderInstance)
  {
  shaderInstance->Begin();
  Texture_Generate(texture, shaderInstance->GetShader());
  shaderInstance->End();
  },
  "texture", "shaderInstance");
static int const Texture2D_GenerateFromShaderInstance_Alias = Function_Alias("Texture2D_GenerateFromShaderInstance", "GenerateFromShader");

static Function const Texture2D_GenerateMipmap_Registration = Function_Bind(
  "Texture2D_GenerateMipmap",
  "Automatically calculate all non-zero mipmap levels of 'texture'",
  [](Texture2D const& texture)
  {
  texture->GenerateMipmap();
  },
  "texture");
static int const Texture2D_GenerateMipmap_Alias = Function_Alias("Texture2D_GenerateMipmap", "GenerateMipmap");

static Function const Texture2D_GetHeight_Registration = Function_Bind(
  "Texture2D_GetHeight",
  "Return the height of 'texture'",
  [](Texture2D const& texture) -> int
  {
  return texture->GetHeight();
  },
  "texture");
static int const Texture2D_GetHeight_Alias = Function_Alias("Texture2D_GetHeight", "GetHeight");

static Function const Texture2D_GetWidth_Registration = Function_Bind(
  "Texture2D_GetWidth",
  "Return the width of 'texture'",
  [](Texture2D const& texture) -> int
  {
  return texture->GetWidth();
  },
  "texture");
static int const Texture2D_GetWidth_Alias = Function_Alias("Texture2D_GetWidth", "GetWidth");

static Function const Texture2D_SaveTo_Registration = Function_Bind(
  "Texture2D_SaveTo",
  "Save 'texture' to a standard image format (bmp, jpg, or png) at 'path'",
  [](Texture2D const& texture, String const& path)
  {
  texture->SaveTo(path);
  },
  "texture", "path");
static int const Texture2D_SaveTo_Alias = Function_Alias("Texture2D_SaveTo", "SaveTo");
