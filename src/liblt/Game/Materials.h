#ifndef Materials_h__
#define Materials_h__

#include "LTE/ShaderInstance.h"

LT_API ShaderInstance Material_Debug();

LT_API ShaderInstance Material_Grass();

LT_API ShaderInstance Material_Ice();

LT_API ShaderInstance Material_Metal();
LT_API ShaderInstance Material_Metal_Diffuse(Texture2D const& diffuse);
LT_API ShaderInstance Material_Metal_Tinted(Texture2D const& diffuse, V3 const& tint);

LT_API ShaderInstance Material_Rock();

LT_API ShaderInstance Material_RockShiny();

LT_API ShaderInstance Material_Water();

#endif
