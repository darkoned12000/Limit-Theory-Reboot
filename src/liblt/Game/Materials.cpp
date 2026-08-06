#include "Materials.h"
#include "ShadingModels.h"
#include "Graphics/Generators.h"

#include "LTE/Location.h"
#include "LTE/Pointer.h"
#include "LTE/Shader.h"
#include "LTE/ShaderInstance.h"
#include "LTE/StackFrame.h"
#include "LTE/Texture2D.h"
#include "LTE/Vec.h"
#include "LTE/FunctionBind.h"

const bool kDebug = false;
const uint kTextureRes = 1024;

namespace {
  Vec<Texture2D, 2> GenerateMetal(float seed) {
    SFRAME("Generate Metal Texture");
    static Shader platingShader;
    static Shader aoShader;
    static Shader dirtShader;
    if (!platingShader) {
      platingShader = Shader_Create("identity.jsl", "gen/plating.jsl");
      platingShader->BindOutput(1, "fragment_color1");
      aoShader = Shader_Create("identity.jsl", "filter_ao.jsl");
      dirtShader = Shader_Create("identity.jsl", "filter_dirt.jsl");
    }

    Texture2D albedoMap = Texture_Create(kTextureRes, kTextureRes, GL_TextureFormat::R8);
    Texture2D normalMap = Texture_Create(kTextureRes, kTextureRes);
    
    (*platingShader)
      ("seed", seed);
    Texture_Generate(albedoMap, normalMap, platingShader);

    (*aoShader)
      ("samples", static_cast<int>(256))
      ("strength", 2.0f)
      ("radius", 30.0f / static_cast<float>(kTextureRes));
    Texture2D t2 = Texture2D_Filter(albedoMap, aoShader);
    albedoMap = Texture2D_Filter(t2, dirtShader);

    Vec<Texture2D, 2> result;
    result[0] = albedoMap;
    result[1] = normalMap;
    return result;
  }
}

ShaderInstance Material_Debug() {
  static ShaderInstance shader;
  if (!shader)
    shader = ShadingModel_Debug();
  return shader;
}
static Function const Material_Debug_Registration = Function_Bind(
  "Material_Debug",
  "None",
  &Material_Debug);



ShaderInstance Material_Grass() {
  static ShaderInstance shader;

  if (!shader) {
    shader = ShaderInstance_Create("identity.jsl", "gen/rock.jsl");
    (*shader)
      ("seed", 1.423f)
      ("colorPower", V3(2.0f, 2.0f, 2.0f));

    Generic<Texture2D> diffuse =
      Generator_ShaderInstance(kTextureRes, kTextureRes, shader);
    shader = ShadingModel_Lambert(diffuse, Generator_NormalMap(diffuse));
  }
  return shader;
}
static Function const Material_Grass_Registration = Function_Bind(
  "Material_Grass",
  "None",
  &Material_Grass);



ShaderInstance Material_Ice() {
  static ShaderInstance shader;
  if (!shader) {
    Generic<Texture2D> ice = Generator_LoadTexture2D(Location_Texture("ice.jpg"));
    Generic<Texture2D> rock = Generator_LoadTexture2D(Location_Texture("rock.jpg"));
    shader = ShadingModel_Fresnel(ice, Generator_NormalMap(rock));
  }
  return shader;
}
static Function const Material_Ice_Registration = Function_Bind(
  "Material_Ice",
  "None",
  &Material_Ice);



ShaderInstance Material_Metal() {
  static ShaderInstance shader;
  if (!shader) {
    Generic<Vec<Texture2D, 2> > g = Cached(Bind(FreeFn(GenerateMetal), 1337.0f));
    if (kDebug)
      shader = ShadingModel_Debug();
    else {
      int data = 0;
      static Texture2D kDefaultDecal = Texture_Create(1, 1, GL_TextureFormat::RGBA8, &data);
      shader = ShadingModel_Metal(Lookup(g, 0), Lookup(g, 1));
      (*shader)
        ("decal", kDefaultDecal);
    }
  }
  return shader;
}
static Function const Material_Metal_Registration = Function_Bind(
  "Material_Metal",
  "None",
  &Material_Metal);



ShaderInstance Material_Rock() {
  static ShaderInstance shader;
  if (!shader) {
    /*Shader rockShader =
      Shader_Create("identity.jsl", "gen/rock.jsl");
    (*rockShader)
      ("seed", 1.423f)
      ("colorPower", V3(1.f, 1.2f, 1.5f)); */
    Generic<Texture2D> texture =
      Generator_LoadTexture2D(Location_Texture("rock.jpg"));
    if (kDebug)
      shader = ShadingModel_Debug();
    else
      shader = ShadingModel_Lambert(texture, Generator_NormalMap(texture));
  }
  return shader;
}
static Function const Material_Rock_Registration = Function_Bind(
  "Material_Rock",
  "None",
  &Material_Rock);



ShaderInstance Material_RockShiny() {
  static Texture2D diffuse;
  static Texture2D normalMap;

  if (!diffuse) {
    diffuse = Texture_LoadFrom(Location_Texture("rock.jpg"));
    normalMap = Generator_NormalMap(diffuse)();
  }

  return ShadingModel_Lambert(diffuse, normalMap);
}
static Function const Material_RockShiny_Registration = Function_Bind(
  "Material_RockShiny",
  "None",
  &Material_RockShiny);



ShaderInstance Material_Water() {
  static ShaderInstance shader;
  if (!shader)
    shader = ShadingModel_Water();
  return shader;
}
static Function const Material_Water_Registration = Function_Bind(
  "Material_Water",
  "None",
  &Material_Water);


