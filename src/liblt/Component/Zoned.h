#ifndef Component_Zoned_h__
#define Component_Zoned_h__

#include "Common.h"
#include "LTE/AutoClass.h"
#include "LTE/SDF.h"

AutoClass(ComponentZoned,
  SDF, region,
  float, fogDensity)

  ComponentZoned() : fogDensity(0) {}

  LT_API float GetContainment(ObjectT*, Position const&);
};

LT_API Object Object_GetZone(Object const& object);

AutoComponent(Zoned)
};

#endif
