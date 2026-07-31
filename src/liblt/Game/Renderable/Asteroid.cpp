#include "../Renderables.h"
#include "LODModel.h"

#include "Game/Materials.h"

#include "LTE/Math.h"
#include "LTE/Model.h"
#include "LTE/SDFs.h"
#include "LTE/SDFMesh.h"

const size_t kUniqueModels = 3;

namespace {
  Renderable MakeLODModel(SDF const& d) {
    Renderable lod0 = Model_Create()->Add(SDFMesh_Create(d), Material_Rock());
    Renderable lod1 = Model_Create()->Add(
      SDFMesh_Create(d, V3(0.5f)), Material_Rock());
    Renderable lod2 = Model_Create()->Add(
      SDFMesh_Create(d, V3(0.25f)), Material_Rock());
    return LODModel_Create(lod0, lod1, lod2);
  }

  Renderable Generate(Renderable_Asteroid_Args const& args) {
    static Renderable models[kUniqueModels];
    if (!models[0]) {
      for (size_t i = 0; i < kUniqueModels; ++i) {
        SDF d = SDF_Radial(
          SDF_FractalWorley(Rand(1, 1000), 6, 2.6f), 0.0f, 2.0f);
        models[i] = MakeLODModel(d);
      }
    }

    return models[args.seed % kUniqueModels];
  }
}

DefineFunction(Renderable_Asteroid) {
  return Generate(args);
}
