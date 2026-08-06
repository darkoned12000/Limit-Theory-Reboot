#ifndef LTE_DrawState_h__
#define LTE_DrawState_h__

#include "CubeMap.h"
#include "Pushable.h"
#include "Stack.h"
#include "Texture2D.h"
#include "Vector.h"

struct DrawState : public Pushable<DrawState> {
  Stack<View*> view;
  Stack<CubeMap> envMap;
  Stack<CubeMap> envMapLF;

  /* Buffer pointers. */
  Texture2D primary;
  Texture2D secondary;
  Texture2D tertiary;

  /* Color. */
  Texture2D color[3];
  Texture2D smallColor[2];

  /* Depth. */
  Texture2D depth;

  /* LOD: set by ComponentDrawable::Draw() from ComponentDrawable::lodLevel.
     Read by LODModel::Render() to select sub-renderable. */
  int lodLevel = 0;

  /* Visibility. */
  Vector<void*> lights;
  Vector<void*> visible;

  void Flip() {
    Swap(primary, secondary);
  }
};

LT_API Data const& DrawState_Get(String const& name);
LT_API void DrawState_Inject(Shader const& shader);

LT_API void DrawState_Link(Shader const& shader);
LT_API void DrawState_Link(ShaderInstance const& shaderState);

LT_API void DrawState_Clear();

LT_API void DrawState_Pop(
  String const& name);

LT_API void DrawState_Push(
  String const& name, Data const& data);

#endif
