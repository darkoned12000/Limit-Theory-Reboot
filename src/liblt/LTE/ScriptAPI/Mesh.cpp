#include "LTE/Function.h"
#include "LTE/FunctionBind.h"
#include "LTE/Mesh.h"

TypeAlias(Reference<MeshT>, Mesh);

DefineConversion(mesh_to_geometry, Mesh, Geometry) {
  dest = (Geometry)src;
}

static Function const Mesh_Center_Registration = Function_Bind(
  "Mesh_Center",
  "Translate 'mesh' such that it is centered about the origin",
  [](Mesh const& mesh)
  {
  mesh->TranslateToCenter();
  },
  "mesh");
static int const Mesh_Center_Alias = Function_Alias("Mesh_Center", "Center");

static Function const Mesh_Create_Registration = Function_Bind(
  "Mesh_Create",
  "Create a new, empty Mesh",
  []() -> Mesh
  {
  return Mesh_Create();
  });

static Function const Mesh_SetOcclusion_Registration = Function_Bind(
  "Mesh_SetOcclusion",
  "Set the occlusion factor of 'mesh' to a constant of 'occlusion'",
  [](Mesh const& mesh, float const& occlusion)
  {
  mesh->SetU(occlusion);
  },
  "mesh", "occlusion");
static int const Mesh_SetOcclusion_Alias = Function_Alias("Mesh_SetOcclusion", "SetOcclusion");
