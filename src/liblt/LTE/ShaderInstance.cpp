#include "ShaderInstance.h"

#include "AutoClass.h"
#include "CubeMap.h"
#include "HashMap.h"
#include "List.h"
#include "Matrix.h"
#include "Pool.h"
#include "Renderer.h"
#include "Shader.h"
#include "Texture2D.h"
#include "Texture3D.h"
#include "V4.h"
#include "LTE/FunctionBind.h"

namespace {
  AutoClass(Cell,
    ListElement<Cell>, next,
    RenderStateSwitch, state)
    Cell() = default;
  };

  int gActiveVersion = -1;
  bool gSkippedState = false;
  ShaderInstanceT* gActiveInstance = nullptr;

  struct ShaderVar {
    virtual ~ShaderVar() = default;
    virtual ShaderVar* Clone() const = 0;
    virtual void Set(int varIndex, ShaderT* shader) = 0;
  };

  using VarMapT = HashMap<int, ShaderVar*>;

  template <class T>
  struct ShaderVarImpl : public ShaderVar {
    using SelfType = ShaderVarImpl;
    Generic<T, void> g;
    POOLED_TYPE

    ShaderVarImpl(Generic<T> const& g) :
      g(g)
      {}

    ShaderVar* Clone() const override {
      return new ShaderVarImpl(*this);
    }

    void Set(int varIndex, ShaderT* shader) override {
      (*shader)(varIndex, g());
    }
  };
}


struct ShaderInstanceData {
  Shader shader;
  VarMapT varMap;
  List<Cell> renderStates;
  int version;
  bool hasBlending;

  ShaderInstanceData(Shader const& shader) :
    shader(shader),
    version(0),
    hasBlending(false)
    {}

  ~ShaderInstanceData() {
    for (VarMapT::iterator it = varMap.begin(); it != varMap.end(); ++it)
      delete it->second;

    for (Cell* curr = renderStates.head; curr;) {
      Cell* next = curr->next;
      delete curr;
      curr = next;
    }
  }
};

static void SetVar(ShaderInstanceT& ss, int index, ShaderVar* var) {
  ss.d->version++;
  if (index < 0) {
    delete var;
    return;
  }

  if (ss.d->varMap.count(index))
    delete ss.d->varMap[index];
  ss.d->varMap[index] = var;
}

static void SetVar(ShaderInstanceT& ss, const char* name, ShaderVar* var) {
  SetVar(ss, ss.d->shader->GetUniformLocation(name), var);
}

static void ApplyRenderState(RenderStateSwitch state) {
  switch (state) {
  case RenderStateSwitch_BlendModeAdditive:
    Renderer_PushBlendMode(BlendMode::Additive); break;
  case RenderStateSwitch_BlendModeAlpha:
    Renderer_PushBlendMode(BlendMode::Alpha); break;
  case RenderStateSwitch_BlendModeComplementary:
    Renderer_PushBlendMode(BlendMode::Complementary); break;
  case RenderStateSwitch_BlendModeDisabled:
    Renderer_PushBlendMode(BlendMode::Disabled); break;
  case RenderStateSwitch_CullModeBackface:
    Renderer_PushCullMode(CullMode::Backface); break;
  case RenderStateSwitch_CullModeDisabled:
    Renderer_PushCullMode(CullMode::Disabled); break;
  case RenderStateSwitch_WireframeOn:
    Renderer_SetWireframe(true); break;
  case RenderStateSwitch_WireframeOff:
    Renderer_SetWireframe(false); break;
  case RenderStateSwitch_ZBufferOn:
    Renderer_PushZBuffer(true); break;
  case RenderStateSwitch_ZBufferOff:
    Renderer_PushZBuffer(false); break;
  case RenderStateSwitch_ZWritableOn:
    Renderer_PushZWritable(true); break;
  case RenderStateSwitch_ZWritableOff:
    Renderer_PushZWritable(false); break;
  default:
    break;
  }
}

static void UndoRenderState(RenderStateSwitch state) {
  switch (state) {
  case RenderStateSwitch_BlendModeAdditive:
    Renderer_PopBlendMode(); break;
  case RenderStateSwitch_BlendModeAlpha:
    Renderer_PopBlendMode(); break;
  case RenderStateSwitch_BlendModeComplementary:
    Renderer_PopBlendMode(); break;
  case RenderStateSwitch_BlendModeDisabled:
    Renderer_PopBlendMode(); break;
  case RenderStateSwitch_CullModeBackface:
    Renderer_PopCullMode(); break;
  case RenderStateSwitch_CullModeDisabled:
    Renderer_PopCullMode(); break;
  case RenderStateSwitch_WireframeOn:
    Renderer_SetWireframe(false); break;
  case RenderStateSwitch_ZBufferOn:
    Renderer_PopZBuffer(); break;
  case RenderStateSwitch_ZBufferOff:
    Renderer_PopZBuffer(); break;
  case RenderStateSwitch_ZWritableOn:
    Renderer_PopZWritable(); break;
  case RenderStateSwitch_ZWritableOff:
    Renderer_PopZWritable(); break;
  default:
    break;
  }
}

ShaderInstanceT::ShaderInstanceT(Shader const& shader) :
  d(new ShaderInstanceData(shader))
  {}

ShaderInstanceT::~ShaderInstanceT() {
  delete d;
}

ShaderInstance ShaderInstanceT::Clone() const {
  ShaderInstance ss = ShaderInstance_Create(d->shader);

  /* Copy the variable map. */
  for (VarMapT::iterator it = d->varMap.begin(); it != d->varMap.end(); ++it)
    ss->d->varMap[it->first] = it->second->Clone();

  /* Copy the linked list of RenderStateSwitches. */
  Cell** prev = &ss->d->renderStates.head;
  Cell* next = d->renderStates.head;
  while (next) {
    *prev = new Cell(*next);
    prev = &(*prev)->next.t;
  }
  *prev = nullptr;

  ss->d->hasBlending = d->hasBlending;
  return ss;
}

void ShaderInstanceT::Begin() {
  /* State caching: skip re-uploading uniforms and re-applying render state
     if the exact same ShaderInstance is being drawn again with no changes.
     Uses an instance pointer (not a global version number) to avoid false
     cache hits between different instances that share the same Shader and
     happen to have the same version count.

     Even on a cache hit, we must re-inject the WVP matrices because the
     world transform changes per-object (set by Renderer_SetWorldTransform
     before each draw). */
  gSkippedState = (this == gActiveInstance) && (d->version == gActiveVersion);

  if (!gSkippedState) {
    Renderer_SetShader(*d->shader);
    gActiveVersion = d->version;
    gActiveInstance = this;

    for (Cell* curr = d->renderStates.head; curr; curr = curr->next)
      ApplyRenderState(curr->state);

    for (VarMapT::iterator it = d->varMap.begin();
         it != d->varMap.end();
         ++it)
      it->second->Set(it->first, d->shader);
  } else {
    /* Cache hit: shader is already bound, but the world matrix may have
       changed since the last draw. Re-upload WVP matrices. */
    Renderer_SetShader(*d->shader);
  }
}

void ShaderInstanceT::End() {
  if (!gSkippedState)
    for (Cell* cell = d->renderStates.head; cell; cell = cell->next)
      UndoRenderState(cell->state);
}

Shader const& ShaderInstanceT::GetShader() const {
  return d->shader;
}

bool ShaderInstanceT::HasBlending() const {
  return d->hasBlending;
}

bool ShaderInstanceT::HasState(RenderStateSwitch state) const {
  for (Cell* cell = d->renderStates.head; cell; cell = cell->next)
    if (cell->state == state)
      return true;
  return false;
}

void ShaderInstanceT::Remove(char const* name) {
  int index = d->shader->GetUniformLocation(name);
  if (d->varMap.count(index)) {
    delete d->varMap[index];
    d->varMap.erase(index);
    d->version++;
  }
}

ShaderInstanceT& ShaderInstanceT::SetCubeMap(
  char const* name,
  Generic<CubeMap> const& g)
{
  SetVar(*this, name, new ShaderVarImpl<CubeMap>(g));
  return *this;
}

ShaderInstanceT& ShaderInstanceT::SetCubeMap(
  int varIndex,
  Generic<CubeMap> const& g)
{
  SetVar(*this, varIndex, new ShaderVarImpl<CubeMap>(g));
  return *this;
}

ShaderInstanceT& ShaderInstanceT::SetFloat(
  char const* name,
  GenericFloat const& g)
{
  SetVar(*this, name, new ShaderVarImpl<float>(g));
  return *this;
}

ShaderInstanceT& ShaderInstanceT::SetFloat(
  int varIndex,
  GenericFloat const& g)
{
  SetVar(*this, varIndex, new ShaderVarImpl<float>(g));
  return *this;
}

ShaderInstanceT& ShaderInstanceT::SetFloat2(
  char const* name,
  GenericV2 const& g)
{
  SetVar(*this, name, new ShaderVarImpl<V2>(g));
  return *this;
}

ShaderInstanceT& ShaderInstanceT::SetFloat2(
  int varIndex,
  GenericV2 const& g)
{
  SetVar(*this, varIndex, new ShaderVarImpl<V2>(g));
  return *this;
}

ShaderInstanceT& ShaderInstanceT::SetFloat3(
  char const* name,
  GenericV3 const& g)
{
  SetVar(*this, name, new ShaderVarImpl<V3>(g));
  return *this;
}

ShaderInstanceT& ShaderInstanceT::SetFloat3(
  int varIndex,
  GenericV3 const& g)
{
  SetVar(*this, varIndex, new ShaderVarImpl<V3>(g));
  return *this;
}

ShaderInstanceT& ShaderInstanceT::SetFloat4(
  char const* name,
  GenericV4 const& g)
{
  SetVar(*this, name, new ShaderVarImpl<V4>(g));
  return *this;
}

ShaderInstanceT& ShaderInstanceT::SetFloat4(
  int varIndex,
  GenericV4 const& g)
{
  SetVar(*this, varIndex, new ShaderVarImpl<V4>(g));
  return *this;
}

ShaderInstanceT& ShaderInstanceT::SetInt(
  char const* name,
  GenericInt const& g)
{
  SetVar(*this, name, new ShaderVarImpl<int>(g));
  return *this;
}

ShaderInstanceT& ShaderInstanceT::SetInt(
  int varIndex,
  GenericInt const& g)
{
  SetVar(*this, varIndex, new ShaderVarImpl<int>(g));
  return *this;
}

ShaderInstanceT& ShaderInstanceT::SetMatrix(
  char const* name,
  Generic<const Matrix*> const& g)
{
  SetVar(*this, name, new ShaderVarImpl<const Matrix*>(g));
  return *this;
}

ShaderInstanceT& ShaderInstanceT::SetMatrix(
  int varIndex,
  Generic<const Matrix*> const& g)
{
  SetVar(*this, varIndex, new ShaderVarImpl<const Matrix*>(g));
  return *this;
}

ShaderInstanceT& ShaderInstanceT::SetState(RenderStateSwitch state) {
  Cell* newCell = new Cell;
  newCell->next = d->renderStates.head;
  newCell->state = state;
  d->renderStates.head = newCell;
  d->version++;

  /* If blending is being turned on, we need to make a note. */
  if (   state == RenderStateSwitch_BlendModeAdditive
      || state == RenderStateSwitch_BlendModeAlpha
      || state == RenderStateSwitch_BlendModeComplementary)
  {
    d->hasBlending = true;
  }
  return *this;
}

ShaderInstanceT& ShaderInstanceT::SetTexture2D(
  char const* name,
  Generic<Texture2D> const& g)
{
  SetVar(*this, name, new ShaderVarImpl<Texture2D>(g));
  return *this;
}

ShaderInstanceT& ShaderInstanceT::SetTexture2D(
  int varIndex,
  Generic<Texture2D> const& g)
{
  SetVar(*this, varIndex, new ShaderVarImpl<Texture2D>(g));
  return *this;
}

ShaderInstanceT& ShaderInstanceT::SetTexture3D(
  char const* name,
  Generic<Texture3D> const& g)
{
  SetVar(*this, name, new ShaderVarImpl<Texture3D>(g));
  return *this;
}

ShaderInstanceT& ShaderInstanceT::SetTexture3D(
  int varIndex,
  Generic<Texture3D> const& g)
{
  SetVar(*this, varIndex, new ShaderVarImpl<Texture3D>(g));
  return *this;
}

ShaderInstance ShaderInstance_Create(Shader const& shader) {
  return new ShaderInstanceT(shader);
}
static Function const ShaderInstance_Create_Registration = Function_Bind(
  "ShaderInstance_Create",
  "None",
  [](Shader const& shader) -> ShaderInstance {
    return ShaderInstance_Create(shader);
  },
  "shader");


