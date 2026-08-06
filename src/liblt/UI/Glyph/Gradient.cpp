#include "../Glyphs.h"

#include "LTE/Pool.h"
#include "LTE/Shader.h"
#include "LTE/V4.h"
#include "LTE/FunctionBind.h"

namespace {
  AutoClass(GradientVertex,
    V3, p,
    V4, color1,
    V4, color2,
    V4, uvsize)
    GradientVertex() = default;
  };

  AutoClassDerived(Gradient, GlyphT, Glyph_Gradient_Args, args)
    DERIVED_TYPE_EX(Gradient)
    POOLED_TYPE

    Gradient() = default;

    Glyph Clone() const override {
      return Glyph_Gradient(args);
    }

    Shader GetShader() const override {
      return Shader_Create("widget.jsl", "ui/gradient.jsl");
    }

    Type GetVertexFormat() const override {
      return Type_Get<GradientVertex>();
    }

    void Submit(void* vvertices, GlyphState const& s) const override {
      GradientVertex* vertices = (GradientVertex*)vvertices;

      V2 ss = args.position * s.scale + s.center;
      V2 size = args.size * s.scale;
      float alpha1 = args.alpha1 * s.alpha;
      float alpha2 = args.alpha2 * s.alpha;
      V4 color1 = V4(alpha1 * args.color1 * s.color, alpha1);
      V4 color2 = V4(alpha2 * args.color2 * s.color, alpha2);

      vertices[0] = GradientVertex(
        V3(ss + V2(0, 0), 0), color1, color2, V4(0, 0, size.x, size.y));
      vertices[1] = GradientVertex(
        V3(ss + V2(size.x, 0), 0), color1, color2, V4(1, 0, size.x, size.y));
      vertices[2] = GradientVertex(
        V3(ss + V2(size.x, size.y), 0), color1, color2, V4(1, 1, size.x, size.y));
      vertices[3] = GradientVertex(
        V3(ss + V2(0, size.y), 0), color1, color2, V4(0, 1, size.x, size.y));
    }

    Glyph Transform(V2 const& offset, V2 const& scale) override {
      args.position = scale * args.position + offset;
      args.size *= scale;
      return this;
    }
  };
}

Glyph Glyph_Gradient(Glyph_Gradient_Args const& args) {
  return new Gradient(args);
}
static Function const Glyph_Gradient_Registration = Function_Bind(
  "Glyph_Gradient",
  "None",
  [](V2 const& position, V2 const& size, Color const& color1, float const& alpha1, Color const& color2, float const& alpha2) -> Glyph { return Glyph_Gradient(position, size, color1, alpha1, color2, alpha2); },
  "position", "size", "color1", "alpha1", "color2", "alpha2");



