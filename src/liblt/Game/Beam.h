#ifndef Beam_h__
#define Beam_h__

#include "Object.h"
#include "ObjectWrapper.h"

#include "Component/Damager.h"
#include "Component/Drawable.h"
#include "Component/Orientation.h"
#include "LTE/Common.h"

using BeamBaseT = ObjectWrapper
  < Component_Damager
  < Component_Drawable
  < Component_Orientation
  < ObjectWrapperTail<ObjectType_Beam>
  > > > >;

struct Beam : public BeamBaseT {
  V3 origin;
  V3 direction;
  float width;
};

LT_API Beam* Beam_Create();

#endif
