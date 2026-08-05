#include "Camera.h"

#include "Component/Motion.h"

#include "LTE/Bound.h"
#include "LTE/Math.h"
#include "LTE/Mouse.h"
#include "LTE/Plane.h"
#include "LTE/Renderer.h"
#include "LTE/Smooth.h"
#include "LTE/Stack.h"
#include "LTE/Vector.h"
#include "LTE/View.h"
#include "LTE/Viewport.h"
#include "LTE/FunctionBind.h"

const float kFovY = Radians(62.0f);
const float kNear = 0.05f;
const float kFar = 1.0e6f;

namespace {
  Vector<Camera>& GetStack() {
    static Vector<Camera> stack;
    return stack;
  }

    AutoClassDerived(CameraImpl, CameraT,
      float, fov,
      Position, relativePos,
      Position, relativeLookAt,
      V3, relativeUp,
      Smooth<Position>, position,
      Smooth<Position>, lookAt,
      Smooth<V3>, up,
      Object, target,
      float, rigidity)

      DERIVED_TYPE_EX(CameraImpl)

      CameraImpl() :
        fov(Radians(62.0f)),
        relativePos(0, 1, -10),
        relativeLookAt(0, 0, 0),
        relativeUp(0, 1, 0),
        position(Position(0, 0, -1)),
        lookAt(Position(0)),
        up(V3(0, 1, 0)),
        rigidity(1)
      {
        Motion.mass = 1;
        Motion.inertia = 1;
      }

      void OnUpdate(UpdateState& state) override {
        BaseType::OnUpdate(state);

        if (target && !target->IsDeleted()) {
          target->GetContainer()->AddInterior(this);
          position.target = relativePos;
          lookAt.target = relativeLookAt;
          up.target = relativeUp;

          Motion.mass = target->GetMass();
          Motion.velocity = target->GetVelocity();
        }

        position.value += Motion.velocity * state.dt;
        position.Update(rigidity * state.dt);
        lookAt.Update(rigidity * state.dt);
        up.Update(rigidity * state.dt);

        V3 newLook = Normalize(lookAt.value - position.value);
        Orientation.GetTransformW() = Transform_LookUp(position, newLook, up);
      }

    Object const& GetTarget() const override {
      return target;
    }

    View GetView(float aspect) const override {
      return View(GetTransform(), fov, aspect, kNear, kFar);
    }

    void SetFov(float fov) override {
      this->fov = fov;
    }

    void SetRelativePos(V3 const& pos) override {
      relativePos = pos;
    }

    void SetRelativeLookAt(V3 const& lookAt) override {
      relativeLookAt = lookAt;
    }

    void SetRelativeUp(V3 const& up) override {
      relativeUp = up;
    }

    void SetRigidity(float rigidity) override {
      this->rigidity = rigidity;
    }

    void SetTarget(Object const& object) override {
      target = object;
    }
  };

  DERIVED_IMPLEMENT(CameraImpl)
}

Camera Camera_Create() {
  return new CameraImpl;
}
static Function const Camera_Create_Registration = Function_Bind(
  "Camera_Create",
  "None",
  &Camera_Create);



bool Camera_CanSee(Object const& object, float const& maxDistance) {
  Camera cam = Camera_Get();
  if (!cam)
    return false;
  float d = Squared(maxDistance);
  return LengthSquared(object->GetPos() - cam->GetPos()) < d;
}
static Function const Camera_CanSee_Registration = Function_Bind(
  "Camera_CanSee",
  "None",
  &Camera_CanSee,
  "object", "maxDistance");



Camera Camera_Get() {
  return GetStack().size() ? GetStack().back() : nullptr;
}
static Function const Camera_Get_Registration = Function_Bind(
  "Camera_Get",
  "None",
  &Camera_Get);



void Camera_Pop() {
  GetStack().pop();
}

void Camera_Push(Camera const& camera) {
  GetStack().push(camera);
}
