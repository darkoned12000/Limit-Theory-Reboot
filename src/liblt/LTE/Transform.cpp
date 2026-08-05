#include "Transform.h"
#include "Math.h"
#include "LTE/FunctionBind.h"

/* TODO : Analytic mult. */
Transform operator*(Transform const& a, Transform const& b) {
  return Transform_Matrix(a.GetMatrix() * b.GetMatrix());
}

Transform Mix(Transform const& a, Transform const& b, double t) {
  Transform self;
  self.pos = Mix(a.pos, b.pos, t);
  self.look = Mix(a.look, b.look, t);
  self.up = Mix(a.up, b.up, t);
  self.scale = Mix(a.scale, b.scale, t);
  self.Orthogonalize();
  return self;
}

static Function const Transform_GetDir_Registration = Function_Bind(
  "Transform_GetDir",
  "Return the direction 'dir' under 'transform'",
  [](Transform const& transform, V3F const& dir) -> V3F
  {
  return transform.TransformDir(dir);
  },
  "transform", "dir");
static int const Transform_GetDir_Alias = Function_Alias("Transform_GetDir", "GetDir");

static Function const Transform_GetVector_Registration = Function_Bind(
  "Transform_GetVector",
  "Return the vector 'vector' under 'transform'",
  [](Transform const& transform, V3F const& vector) -> V3F
  {
  return transform.TransformVector(vector);
  },
  "transform", "vector");
static int const Transform_GetVector_Alias = Function_Alias("Transform_GetVector", "GetVector");

static Function const Transform_GetPoint_Registration = Function_Bind(
  "Transform_GetPoint",
  "Return the point 'point' under 'transform'",
  [](Transform const& transform, V3D const& point) -> V3F
  {
  return transform.TransformPoint(point);
  },
  "transform", "point");
static int const Transform_GetPoint_Alias = Function_Alias("Transform_GetPoint", "GetPoint");

static Function const Transform_GetInverseDir_Registration = Function_Bind(
  "Transform_GetInverseDir",
  "Return the direction that, under 'transform', yields 'dir",
  [](Transform const& transform, V3F const& dir) -> V3F
  {
  return transform.InverseDir(dir);
  },
  "transform", "dir");
static int const Transform_GetInverseDir_Alias = Function_Alias("Transform_GetInverseDir", "GetInverseDir");

Transform Transform_Identity() {
  return Transform(0, V3F(1, 0, 0), V3F(0, 1, 0), V3F(0, 0, 1), V3F(1));
}
static Function const Transform_Identity_Registration = Function_Bind(
  "Transform_Identity",
  "None",
  &Transform_Identity);



/* TODO : Analytic inverse. */
Transform Transform_Inverse(Transform const& source) {
  return Transform_Matrix(source.GetMatrix().Inverse());
}
static Function const Transform_Inverse_Registration = Function_Bind(
  "Transform_Inverse",
  "None",
  &Transform_Inverse,
  "source");



Transform Transform_Look(V3D const& pos, V3F const& look) {
  Transform self(pos, V3F(1, 0, 0), OrthoVector(look), look, V3F(1));
  self.Orthogonalize();
  return self;
}
static Function const Transform_Look_Registration = Function_Bind(
  "Transform_Look",
  "None",
  &Transform_Look,
  "pos", "look");



Transform Transform_LookUp(V3D const& pos, V3F const& look, V3F const& up) {
  Transform self(pos, V3F(1, 0, 0), up, look, V3F(1));
  self.Orthogonalize();
  return self;
}
static Function const Transform_LookUp_Registration = Function_Bind(
  "Transform_LookUp",
  "None",
  &Transform_LookUp,
  "pos", "look", "up");



Transform Transform_Matrix(MatrixD const& matrix) {
  V3D pos(matrix[12], matrix[13], matrix[14]);
  V3F right((float)matrix[0], (float)matrix[1], (float)matrix[2]);
  V3F up((float)matrix[4], (float)matrix[5], (float)matrix[6]);
  V3F look((float)matrix[8], (float)matrix[9], (float)matrix[10]);
  V3F scale(Length(right), Length(up), Length(look));
  return Transform(pos, right / scale.x, up / scale.y, look / scale.z, scale);
}
static Function const Transform_Matrix_Registration = Function_Bind(
  "Transform_Matrix",
  "None",
  &Transform_Matrix,
  "matrix");



void Transform_Rotate(Transform const& source, V3F const& rotation) {
  RotateBasis(
    Mutable(source.right),
    Mutable(source.up),
    Mutable(source.look),
    rotation);
}
static Function const Transform_Rotate_Registration = Function_Bind(
  "Transform_Rotate",
  "None",
  &Transform_Rotate,
  "source", "rotation");
static int const Transform_Rotate_Alias = Function_Alias("Transform_Rotate", "Rotate");



Transform Transform_Scale(V3F const& scale) {
  return Transform(0, V3F(1, 0, 0), V3F(0, 1, 0), V3F(0, 0, 1), scale);
}
static Function const Transform_Scale_Registration = Function_Bind(
  "Transform_Scale",
  "None",
  &Transform_Scale,
  "scale");



Transform Transform_Translation(V3D const& pos) {
  return Transform(pos, V3F(1, 0, 0), V3F(0, 1, 0), V3F(0, 0, 1), V3F(1));
}
static Function const Transform_Translation_Registration = Function_Bind(
  "Transform_Translation",
  "None",
  &Transform_Translation,
  "pos");



Transform Transform_ST(V3F const& scale, V3D const& pos) {
  return Transform_Translation(pos) * Transform_Scale(scale);
}
static Function const Transform_ST_Registration = Function_Bind(
  "Transform_ST",
  "None",
  &Transform_ST,
  "scale", "pos");


