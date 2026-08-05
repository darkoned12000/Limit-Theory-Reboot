#include "../Glyphs.h"

#include "LTE/Pool.h"
#include "LTE/Shader.h"
#include "LTE/V4.h"
#include "LTE/FunctionBind.h"

namespace {
  AutoClass(GridVertex,
    V3, p,
    V4, color,
    V4, uvSize,
    V4, offsetScale)
    GridVertex() = default;
  };

  AutoClassDerived(Grid, GlyphT, Glyph_Grid_Args, args)
    DERIVED_TYPE_EX(Grid)
    POOLED_TYPE

    Grid() = default;

    Glyph Clone() const override {
      return Glyph_Grid(args);
    }

    Shader GetShader() const override {
      return Shader_Create("widget.jsl", "ui/grid.jsl");
    }

    Type GetVertexFormat() const override {
      return Type_Get<GridVertex>();
    }

    void Submit(void* vvertices, GlyphState const& s) const override {
      GridVertex* vertices = (GridVertex*)vvertices;

      V2 ss1 = args.p1 * s.scale + s.center;
      V2 ss2 = args.p2 * s.scale + s.center;
      V2 size = (ss2 - ss1);
      float alpha = args.alpha * s.alpha;
      V4 color = V4(alpha * args.color * s.color, alpha);
      V4 os = V4(args.offset.x, args.offset.y, args.scale.x, args.scale.y);

      vertices[0] = GridVertex(V3(ss1 + size * V2(0, 0), 0), color, V4(0, 0, size.x, size.y), os);
      vertices[1] = GridVertex(V3(ss1 + size * V2(1, 0), 0), color, V4(1, 0, size.x, size.y), os);
      vertices[2] = GridVertex(V3(ss1 + size * V2(1, 1), 0), color, V4(1, 1, size.x, size.y), os);
      vertices[3] = GridVertex(V3(ss1 + size * V2(0, 1), 0), color, V4(0, 1, size.x, size.y), os);
    }

    Glyph Transform(V2 const& offset, V2 const& scale) override {
      args.p1 = scale * args.p1 + offset;
      args.p2 = scale * args.p2 + offset;
      return this;
    }
  };
}

Glyph Glyph_Grid(Glyph_Grid_Args const& args) {
  return new Grid(args);
}
static Function const Glyph_Grid_Registration = Function_Bind(
  "Glyph_Grid",
  "None",
  [](V2 const& p1, V2 const& p2, Color const& color, float const& alpha, V2 const& offset, V2 const& scale) -> Glyph { return Glyph_Grid(p1, p2, color, alpha, offset, scale); },
  "p1", "p2", "color", "alpha", "offset", "scale");


