#ifndef LTE_Model_h__
#define LTE_Model_h__

#include "Geometry.h"
#include "Renderable.h"

struct ModelT : public RenderableT {
  DERIVED_TYPE_EX(ModelT)
  struct ModelData& d;
  short version;

  LT_API ModelT();
  LT_API ~ModelT() override;

  size_t GetHash() const override {
    return (size_t)this;
  }

  short GetVersion() const override {
    return version;
  }

  LT_API Model Add(
    Geometry const& geom,
    ShaderInstance const& shader,
    bool collidable = true);

  LT_API Model Add(Model const& model);

  LT_API void Render(DrawState* state) const override;

  LT_API void RenderPiece(size_t piece, DrawState* state) const;

  /* Instanced render: draws all entries of this model with a single instanced
     draw call per entry. Caller must call Renderer_BeginInstancedDraw before
     this and Renderer_EndInstancedDraw after. */
  LT_API void RenderInstanced(DrawState* state, int instanceCount) const;

  LT_API Bound3 GetBound() const override;
  
  LT_API Mesh GetCollisionMesh() const override;

  LT_API bool Intersects(
    Ray const& r,
    float* tOut = nullptr,
    V3* normalOut = nullptr) const override;
  
  LT_API V3 Sample() const override;

  /* NOTE : Need to call UpdateVersion every time you change one of the internal
            rebderables once it is already added to the model! Model cannot
            automatically tell that it has changed. */
  LT_API Model UpdateVersion();
};

inline Model Model_Create() {
  return new ModelT;
}

#endif
