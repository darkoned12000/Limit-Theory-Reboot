#include "../Glyphs.h"

#include "LTE/Pool.h"
#include "LTE/Shader.h"
#include "LTE/V4.h"
#include "LTE/FunctionBind.h"

namespace {
  AutoClass(RectVertex,
    V3, p,
    V4, color,
    V4, uvsize,
    V2, bevelvariance)
    RectVertex() = default;
  };

  AutoClassDerived(Rect, GlyphT, Glyph_Rect_Args, args)
    DERIVED_TYPE_EX(Rect)
    POOLED_TYPE

    Rect() = default;

    Glyph Clone() const override {
      return Glyph_Rect(args);
    }

    Shader GetShader() const override {
      return Shader_Create("widget.jsl", "ui/rect.jsl");
    }

    Type GetVertexFormat() const override {
      return Type_Get<RectVertex>();
    }

    void Submit(void* vvertices, GlyphState const& s) const override {
      RectVertex* vertices = (RectVertex*)vvertices;

      V2 ss = args.position * s.scale + s.center;
      V2 size = args.size * s.scale + V2(48);
      float alpha = args.alpha * s.alpha;
      V4 color = V4(alpha * args.color * s.color, alpha);
      V2 bv(args.bevel, args.variance);

      vertices[0] = RectVertex(
        V3(ss + V2(-size.x, -size.y), 0), color, V4(0, 0, size.x, size.y), bv);
      vertices[1] = RectVertex(
        V3(ss + V2( size.x, -size.y), 0), color, V4(1, 0, size.x, size.y), bv);
      vertices[2] = RectVertex(
        V3(ss + V2( size.x,  size.y), 0), color, V4(1, 1, size.x, size.y), bv);
      vertices[3] = RectVertex(
        V3(ss + V2(-size.x,  size.y), 0), color, V4(0, 1, size.x, size.y), bv);
    }

    Glyph Transform(V2 const& offset, V2 const& scale) override {
      args.position = scale * args.position + offset;
      args.size *= scale;
      return this;
    }
  };
}

Glyph Glyph_Rect(Glyph_Rect_Args const& args) {
  return new Rect(args);
}
static Function const Glyph_Rect_Registration = Function_Bind(
  "Glyph_Rect",
  "None",
  [](V2 const& position, V2 const& size, Color const& color, float const& alpha, float const& bevel, float const& variance) -> Glyph { return Glyph_Rect(position, size, color, alpha, bevel, variance); },
  "position", "size", "color", "alpha", "bevel", "variance");



