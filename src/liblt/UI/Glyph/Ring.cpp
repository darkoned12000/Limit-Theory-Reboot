#include "../Glyphs.h"

#include "LTE/Pool.h"
#include "LTE/Shader.h"
#include "LTE/V4.h"
#include "LTE/FunctionBind.h"

namespace {
  AutoClass(RingVertex,
    V3, p,
    V4, color,
    V4, uvRadiusAngle)
    RingVertex() = default;
  };

  AutoClassDerived(Ring, GlyphT, Glyph_Ring_Args, args)
    DERIVED_TYPE_EX(Ring)
    POOLED_TYPE

    Ring() = default;

    Glyph Clone() const override {
      return Glyph_Ring(args);
    }

    Shader GetShader() const override {
      return Shader_Create("widget.jsl", "ui/ring.jsl");
    }

    Type GetVertexFormat() const override {
      return Type_Get<RingVertex>();
    }

    void Submit(void* vvertices, GlyphState const& s) const override {
      RingVertex* vertices = (RingVertex*)vvertices;

      V2 ss = args.position * s.scale + s.center;
      float r = 4.0f * args.radius * s.scale.GetMin();
      float a = args.angle;
      float alpha = args.alpha * s.alpha;
      V4 color = V4(alpha * args.color * s.color, alpha);

      vertices[0] = RingVertex(V3(ss + V2(-r, -r), 0), color, V4(0, 0, r, a));
      vertices[1] = RingVertex(V3(ss + V2( r, -r), 0), color, V4(1, 0, r, a));
      vertices[2] = RingVertex(V3(ss + V2( r,  r), 0), color, V4(1, 1, r, a));
      vertices[3] = RingVertex(V3(ss + V2(-r,  r), 0), color, V4(0, 1, r, a));
    }

    Glyph Transform(V2 const& offset, V2 const& scale) override {
      args.position = scale * args.position + offset;
      args.radius *= scale.GetMin();
      return this;
    }
  };
}

Glyph Glyph_Ring(Glyph_Ring_Args const& args) {
  return new Ring(args);
}
static Function const Glyph_Ring_Registration = Function_Bind(
  "Glyph_Ring",
  "None",
  [](V2 const& position, float const& radius, Color const& color, float const& alpha, float const& angle) -> Glyph { return Glyph_Ring(position, radius, color, alpha, angle); },
  "position", "radius", "color", "alpha", "angle");


