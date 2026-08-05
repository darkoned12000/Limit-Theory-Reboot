#include "../Glyphs.h"

#include "LTE/Pool.h"
#include "LTE/Shader.h"
#include "LTE/V4.h"
#include "LTE/FunctionBind.h"

const float kExpansion = 32;

namespace {
  AutoClass(TriangleVertex,
    V3, p,
    V4, p1p2,
    V4, color,
    V4, uvp3)
    TriangleVertex() = default;
  };

  AutoClassDerived(Triangle, GlyphT, Glyph_Triangle_Args, args)
    DERIVED_TYPE_EX(Triangle)
    POOLED_TYPE

    Triangle() = default;

    Glyph Clone() const override {
      return Glyph_Triangle(args);
    }

    Shader GetShader() const override {
      return Shader_Create("widget.jsl", "ui/triangle.jsl");
    }

    Type GetVertexFormat() const override {
      return Type_Get<TriangleVertex>();
    }

    void Submit(void* vvertices, GlyphState const& s) const override {
      TriangleVertex* vertices = (TriangleVertex*)vvertices;

      V2 p1 = args.p1 * s.scale + s.center;
      V2 p2 = args.p2 * s.scale + s.center;
      V2 p3 = args.p3 * s.scale + s.center;
      V2 ss1 = Min(p1, Min(p2, p3)) - V2(kExpansion);
      V2 ss2 = Max(p1, Max(p2, p3)) + V2(kExpansion);
      V2 size = ss2 - ss1;
      float alpha = args.alpha * s.alpha;
      V4 color = V4(alpha * args.color * s.color, alpha);

      vertices[0] = TriangleVertex(
        V3(ss1 + V2(0, 0), 0), V4(p1.x, p1.y, p2.x, p2.y), color, V4(0, 0, p3.x, p3.y));
      vertices[1] = TriangleVertex(
        V3(ss1 + V2(size.x, 0), 0), V4(p1.x, p1.y, p2.x, p2.y), color, V4(1, 0, p3.x, p3.y));
      vertices[2] = TriangleVertex(
        V3(ss1 + V2(size.x, size.y), 0), V4(p1.x, p1.y, p2.x, p2.y), color, V4(1, 1, p3.x, p3.y));
      vertices[3] = TriangleVertex(
        V3(ss1 + V2(0, size.y), 0), V4(p1.x, p1.y, p2.x, p2.y), color, V4(0, 1, p3.x, p3.y));
    }

    Glyph Transform(V2 const& offset, V2 const& scale) override {
      args.p1 = scale * args.p1 + offset;
      args.p2 = scale * args.p2 + offset;
      args.p3 = scale * args.p3 + offset;
      return this;
    }
  };
}

Glyph Glyph_Triangle(Glyph_Triangle_Args const& args) {
  return new Triangle(args);
}
static Function const Glyph_Triangle_Registration = Function_Bind(
  "Glyph_Triangle",
  "None",
  [](V2 const& p1, V2 const& p2, V2 const& p3, Color const& color, float const& alpha) -> Glyph { return Glyph_Triangle(p1, p2, p3, color, alpha); },
  "p1", "p2", "p3", "color", "alpha");


