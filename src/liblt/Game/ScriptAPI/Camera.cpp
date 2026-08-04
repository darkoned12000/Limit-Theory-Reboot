#include "Game/Camera.h"
#include "LTE/Function.h"
#include "LTE/FunctionBind.h"
#include "LTE/View.h"
#include "LTE/Viewport.h"
#include "LTE/Window.h"

TypeAlias(Reference<CameraT>, Camera);

DefineConversion(camera_to_object, Camera, Object) {
  dest = (Object)src;
}

static Function const Camera_GetRay_Registration = Function_Bind(
  "Camera_GetRay",
  "Return the world-space ray that projects to 'position' under 'camera'",
  [](Camera const& camera, V2 const& position) -> RayD
  {
  return camera->GetView(Viewport_Get()->GetAspect()).GetRay(Viewport_Get()->ToNDC(position));
  },
  "camera", "position");
static int const Camera_GetRay_Alias = Function_Alias("Camera_GetRay", "GetRay");

static Function const Camera_GetTarget_Registration = Function_Bind(
  "Camera_GetTarget",
  "Return the object that 'camera' is tracking",
  [](Camera const& camera) -> Object
  {
  return camera->GetTarget();
  },
  "camera");
static int const Camera_GetTarget_Alias = Function_Alias("Camera_GetTarget", "GetTarget");

static Function const Camera_Project_Registration = Function_Bind(
  "Camera_Project",
  "Return the projected position of 'position' from the viewpoint of 'camera'",
  [](Camera const& camera, Position const& position) -> Position
  {
  return camera->GetView(Window_Get()->GetAspect()).Project(position);
  },
  "camera", "position");
static int const Camera_Project_Alias = Function_Alias("Camera_Project", "Project");

static Function const Camera_Pop_Registration = Function_Bind(
  "Camera_Pop",
  "Pop the last camera from the global camera stack",
  []()
  {
  Camera_Pop();
  });

static Function const Camera_Push_Registration = Function_Bind(
  "Camera_Push",
  "Push 'camera' to the global camera stack",
  [](Camera const& camera)
  {
  Camera_Push(camera);
  },
  "camera");
static int const Camera_Push_Alias = Function_Alias("Camera_Push", "Push");

static Function const Camera_SetFov_Registration = Function_Bind(
  "Camera_SetFov",
  "Set 'camera's vertical field of view to 'fov' degrees",
  [](Camera const& camera, float const& fov)
  {
  camera->SetFov(Radians(fov));
  },
  "camera", "fov");
static int const Camera_SetFov_Alias = Function_Alias("Camera_SetFov", "SetFov");

static Function const Camera_SetRelativePos_Registration = Function_Bind(
  "Camera_SetRelativePos",
  "Set 'camera's position to 'position' relative to the target",
  [](Camera const& camera, Position const& position)
  {
  camera->SetRelativePos(position);
  },
  "camera", "position");
static int const Camera_SetRelativePos_Alias = Function_Alias("Camera_SetRelativePos", "SetRelativePos");

static Function const Camera_SetRelativeLookAt_Registration = Function_Bind(
  "Camera_SetRelativeLookAt",
  "Set 'camera' to look at 'position' relative to the target",
  [](Camera const& camera, Position const& lookAt)
  {
  camera->SetRelativeLookAt(lookAt);
  },
  "camera", "lookAt");
static int const Camera_SetRelativeLookAt_Alias = Function_Alias("Camera_SetRelativeLookAt", "SetRelativeLookAt");

static Function const Camera_SetRelativeUp_Registration = Function_Bind(
  "Camera_SetRelativeUp",
  "Set 'camera's up direction 'up' relative to the target",
  [](Camera const& camera, V3 const& up)
  {
  camera->SetRelativeUp(up);
  },
  "camera", "up");
static int const Camera_SetRelativeUp_Alias = Function_Alias("Camera_SetRelativeUp", "SetRelativeUp");

static Function const Camera_SetRigidity_Registration = Function_Bind(
  "Camera_SetRigidity",
  "Set 'camera's to have 'rigidity'",
  [](Camera const& camera, float const& rigidity)
  {
  camera->SetRigidity(rigidity);
  },
  "camera", "rigidity");
static int const Camera_SetRigidity_Alias = Function_Alias("Camera_SetRigidity", "SetRigidity");

static Function const Camera_SetTarget_Registration = Function_Bind(
  "Camera_SetTarget",
  "Set 'camera' to track 'target'",
  [](Camera const& camera, Object const& target)
  {
  camera->SetTarget(target);
  },
  "camera", "target");
static int const Camera_SetTarget_Alias = Function_Alias("Camera_SetTarget", "SetTarget");

static Function const Position_Frame_Registration = Function_Bind(
  "Position_Frame",
  "Return the screen-space coordinate of the projected 'position'",
  [](Position const& position) -> V2
  {
  return Viewport_Get()->FromNDC(position.GetXY());
  },
  "position");
