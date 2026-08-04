#include "LTE/Function.h"
#include "LTE/FunctionBind.h"
#include "LTE/Mesh.h"
#include "LTE/PlateMesh.h"
#include "LTE/Warp.h"

TypeAlias(Reference<PlateMeshT>, PlateMesh);

static Function const PlateMesh_AddPlate_Registration = Function_Bind(
  "PlateMesh_AddPlate",
  "Add a plate with 'center,' 'scale,' 'rotation,' and 'bevel' to 'plateMesh'",
  [](PlateMesh const& plateMesh, V3 const& center, V3 const& scale, V3 const& rotation, float const& bevel)
  {
  plateMesh->Add(center, scale, rotation, bevel);
  },
  "plateMesh", "center", "scale", "rotation", "bevel");
static int const PlateMesh_AddPlate_Alias = Function_Alias("PlateMesh_AddPlate", "Add");

static Function const PlateMesh_AddPlateMesh_Registration = Function_Bind(
  "PlateMesh_AddPlateMesh",
  "Add 'source' to 'plateMesh' with translation 'offset' and scale 'scale'",
  [](PlateMesh const& plateMesh, PlateMesh const& source, V3 const& offset, V3 const& scale)
  {
  plateMesh->Add(source, offset, scale);
  },
  "plateMesh", "source", "offset", "scale");
static int const PlateMesh_AddPlateMesh_Alias = Function_Alias("PlateMesh_AddPlateMesh", "Add");

static Function const PlateMesh_AddWarp_Registration = Function_Bind(
  "PlateMesh_AddWarp",
  "Add 'warp' to 'plateMesh'",
  [](PlateMesh const& plateMesh, Warp const& warp)
  {
  plateMesh->Add(warp);
  },
  "plateMesh", "warp");
static int const PlateMesh_AddWarp_Alias = Function_Alias("PlateMesh_AddWarp", "Add");

static Function const PlateMesh_GetMesh_Registration = Function_Bind(
  "PlateMesh_GetMesh",
  "Create a mesh using 'plateMesh'",
  [](PlateMesh const& plateMesh) -> Mesh
  {
  return plateMesh->GetMesh();
  },
  "plateMesh");
static int const PlateMesh_GetMesh_Alias = Function_Alias("PlateMesh_GetMesh", "GetMesh");

static Function const PlateMesh_ReflectX_Registration = Function_Bind(
  "PlateMesh_ReflectX",
  "Reflect 'plateMesh' over the X axis",
  [](PlateMesh const& plateMesh)
  {
  plateMesh->ReflectX();
  },
  "plateMesh");
static int const PlateMesh_ReflectX_Alias = Function_Alias("PlateMesh_ReflectX", "ReflectX");

static Function const PlateMesh_ReflectY_Registration = Function_Bind(
  "PlateMesh_ReflectY",
  "Reflect 'plateMesh' over the Y axis",
  [](PlateMesh const& plateMesh)
  {
  plateMesh->ReflectY();
  },
  "plateMesh");
static int const PlateMesh_ReflectY_Alias = Function_Alias("PlateMesh_ReflectY", "ReflectY");

static Function const PlateMesh_ReflectZ_Registration = Function_Bind(
  "PlateMesh_ReflectZ",
  "Reflect 'plateMesh' over the Z axis",
  [](PlateMesh const& plateMesh)
  {
  plateMesh->ReflectZ();
  },
  "plateMesh");
static int const PlateMesh_ReflectZ_Alias = Function_Alias("PlateMesh_ReflectZ", "ReflectZ");
