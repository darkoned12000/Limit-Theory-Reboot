#include "RenderPasses.h"

#include "DrawState.h"
#include "Math.h"
#include "Renderer.h"
#include "ShaderInstance.h"
#include "StackFrame.h"
#include "Texture2D.h"
#include "LTE/FunctionBind.h"

namespace {
  void DrawSecondaryFSQ(DrawState* state) {
    state->secondary->Bind(0);
    Renderer_DrawFSQ();
    state->secondary->Unbind();
    state->Flip();
  }

  struct RenderPassPost : public RenderPassT {
    String shaderPath;
    String name;
    Shader shader;
    ShaderInstance shaderInstance;
    DERIVED_TYPE_EX(RenderPassPost)

    RenderPassPost() = default;

    RenderPassPost(String const& vertPath, String const& fragPath) :
      shaderPath(fragPath),
      name("Effect <" + shaderPath + ">"),
      shader(Shader_Create(vertPath, fragPath))
    {
      shaderInstance = ShaderInstance_Create(shader);
    }

    RenderPassPost(String const& fragPath) :
      shaderPath(fragPath),
      name("Effect <" + fragPath + ">")
    {
      shader = Shader_Create("identity.jsl", fragPath);
      shaderInstance = ShaderInstance_Create(shader);
    }

    char const* GetName() const override {
      return name.c_str();
    }

    ShaderInstanceT* GetShader() {
      return shaderInstance;
    }

    void OnRender(DrawState* state) override {
      shaderInstance->Begin();
      DrawState_Link(shader);
      int seedLoc = shader->QueryUniformLocation("seed");
      if (seedLoc >= 0)
        shader->SetFloat(seedLoc, Rand());
      (*shader)
        ("texture", state->primary);
      Renderer_SetShader(*shader);
      DrawSecondaryFSQ(state);
      shaderInstance->End();
    }
  };

  struct Aberration : public RenderPassPost {
    float strength;

    Aberration(float strength) :
      RenderPassPost("post/aberration.jsl"),
      strength(strength)
      {}

    void OnRender(DrawState* state) override {
      (*shader)("strength", strength);
      RenderPassPost::OnRender(state);
    }
  };
}

RenderPass RenderPass_Aberration(float const& strength) {
  return new Aberration(strength);
}
static Function const RenderPass_Aberration_Registration = Function_Bind(
  "RenderPass_Aberration",
  "None",
  &RenderPass_Aberration,
  "strength");



RenderPass RenderPass_PostFilter(String const& shaderPath) {
  return new RenderPassPost(shaderPath);
}
static Function const RenderPass_PostFilter_Registration = Function_Bind(
  "RenderPass_PostFilter",
  "None",
  &RenderPass_PostFilter,
  "shaderPath");



RenderPass RenderPass_Tonemap() {
  return RenderPass_PostFilter("post/tonemap.jsl");
}
static Function const RenderPass_Tonemap_Registration = Function_Bind(
  "RenderPass_Tonemap",
  "None",
  &RenderPass_Tonemap);


