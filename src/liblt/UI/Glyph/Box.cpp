#include "../Glyphs.h"

#include "LTE/Pool.h"
#include "LTE/Shader.h"
#include "LTE/V4.h"
#include "LTE/FunctionBind.h"

namespace {
  AutoClass(BoxVertex,
    V3, p,
    V4, color,
    V4, uvsize)
    BoxVertex() = default;
  };

  AutoClassDerived(Box, GlyphT, Glyph_Box_Args, args)
    DERIVED_TYPE_EX(Box)
    POOLED_TYPE

    Box() = default;

    Glyph Clone() const override {
      return Glyph_Box(args);
    }

    Shader GetShader() const override {
      return Shader_Create("widget.jsl", "ui/box.jsl");
    }

    Type GetVertexFormat() const override {
      return Type_Get<BoxVertex>();
    }

    void Submit(void* vvertices, GlyphState const& s) const override {
      BoxVertex* vertices = (BoxVertex*)vvertices;

      V2 ss = args.position * s.scale + s.center;
      V2 size = args.size * s.scale + V2(48);
      float alpha = args.alpha * s.alpha;
      V4 color = V4(alpha * args.color * s.color, alpha);

      vertices[0] = BoxVertex(
        V3(ss + V2(-size.x, -size.y), 0), color, V4(0, 0, size.x, size.y));
      vertices[1] = BoxVertex(
        V3(ss + V2( size.x, -size.y), 0), color, V4(1, 0, size.x, size.y));
      vertices[2] = BoxVertex(
        V3(ss + V2( size.x,  size.y), 0), color, V4(1, 1, size.x, size.y));
      vertices[3] = BoxVertex(
        V3(ss + V2(-size.x,  size.y), 0), color, V4(0, 1, size.x, size.y));
    }

    Glyph Transform(V2 const& offset, V2 const& scale) override {
      args.position = scale * args.position + offset;
      args.size *= scale;
      return this;
    }
  };
}

Glyph Glyph_Box(Glyph_Box_Args const& args) {
  return new Box(args);
}
static Function const Glyph_Box_Registration = Function_Bind(
  "Glyph_Box",
  "None",
  [](V2 const& position, V2 const& size, Color const& color, float const& alpha) -> Glyph { return Glyph_Box(position, size, color, alpha); },
  "position", "size", "color", "alpha");


