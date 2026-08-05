#include "../Glyphs.h"

#include "LTE/Pool.h"
#include "LTE/Shader.h"
#include "LTE/V4.h"
#include "LTE/FunctionBind.h"

namespace {
  AutoClass(CircleVertex, 
    V3, p,
    V4, color,
    V2, uv)
    CircleVertex() = default;
  };

  AutoClassDerived(Circle, GlyphT, Glyph_Circle_Args, args)
    DERIVED_TYPE_EX(Circle)
    POOLED_TYPE

    Circle() = default;

    Glyph Clone() const override {
      return Glyph_Circle(args);
    }

    Shader GetShader() const override {
      return Shader_Create("widget.jsl", "ui/circle.jsl");
    }

    Type GetVertexFormat() const override {
      return Type_Get<CircleVertex>();
    }

    void Submit(void* vvertices, GlyphState const& s) const override {
      CircleVertex* vertices = (CircleVertex*)vvertices;

      V2 ss = args.position * s.scale + s.center;
      float r = 4.0f * args.radius * s.scale.GetMin();
      float alpha = args.alpha * s.alpha;
      V4 color = V4(alpha * args.color * s.color, alpha);

      vertices[0] = CircleVertex(V3(ss + V2(-r, -r), 0), color, V2(0, 0));
      vertices[1] = CircleVertex(V3(ss + V2( r, -r), 0), color, V2(1, 0));
      vertices[2] = CircleVertex(V3(ss + V2( r,  r), 0), color, V2(1, 1));
      vertices[3] = CircleVertex(V3(ss + V2(-r,  r), 0), color, V2(0, 1));
    }

    Glyph Transform(V2 const& offset, V2 const& scale) override {
      args.position = scale * args.position + offset;
      args.radius *= scale.GetMin();
      return this;
    }
  };
}

Glyph Glyph_Circle(Glyph_Circle_Args const& args) {
  return new Circle(args);
}
static Function const Glyph_Circle_Registration = Function_Bind(
  "Glyph_Circle",
  "None",
  [](V2 const& position, float const& radius, Color const& color, float const& alpha) -> Glyph { return Glyph_Circle(position, radius, color, alpha); },
  "position", "radius", "color", "alpha");


