#include "../Generators.h"

#include "LTE/CubeMap.h"
#include "LTE/ShaderInstance.h"
#include "LTE/StackFrame.h"
#include "LTE/FunctionBind.h"

namespace {
  CubeMap Generate(Generator_Blur_Args const& args) {
    SFRAME("Generate Blurred CubeMap");
    static Shader shader = Shader_Create("identity.jsl", "cubemap/blur.jsl");

    CubeMap const& source = args.source();
    CubeMap self = CubeMap_Create(args.resolution, source->GetFormat());
    (*shader)
      ("radius", args.radius)
      ("source", args.source())
      ("samples", static_cast<int>(args.samples));
    self->GenerateFromShader(shader);
    return self;
  }
}

Generic<CubeMap> Generator_Blur(Generator_Blur_Args const& args) {
  return Cached(Bind(FreeFn(Generate), Generator_Blur_Args(args)));
}
static Function const Generator_Blur_Registration = Function_Bind(
  "Generator_Blur",
  "None",
  [](Generic<CubeMap> const& source, float const& radius, size_t const& resolution, size_t const& samples) -> Generic<CubeMap> { return Generator_Blur(source, radius, resolution, samples); },
  "source", "radius", "resolution", "samples");


