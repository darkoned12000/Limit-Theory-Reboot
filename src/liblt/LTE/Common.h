#ifndef LTE_Common_h__
#define LTE_Common_h__

#include "../Common.h"

/* Container. */
template <class T1> struct Array;
template <class T1> struct AutoPtr;
template <class T1> struct Distribution;
template <class T1> struct List;
template <class T1> struct Pointer;
template <class T1> struct Reference;
template <class T1> struct RingBuffer;
template <class T1> struct Vector;

template <class T1, class T2> struct Map;
template <class T1, class T2> struct Tuple2;
template <class T1, class T2> struct VectorMap;
template <class T1, class T2, class T3> struct Tuple3;

template <class T1, char const* (*)(int)> struct Enum;
template <class T1, int MaxElements> struct Stack;

using ListNP = Reference<struct ListNPT>;

/* Engine. */
using StringList = Reference<struct StringListT>;
using StringTree = Reference<struct StringTreeT>;

/* Math. */
struct Capsule;
struct Cylinder;
struct Segment;
struct SpatialPartition;
struct Sphere;
struct Transform;
struct View;

using CollisionMesh = Reference<struct CollisionMeshT>;
using RNG = Reference<struct RNGT>;
using SDF = Reference<struct SDFT>;
using Warp = Reference<struct WarpT>;

/* Math - Parametric. */
template <class T1> struct V2T;
using V2 = V2T<float>;
using V2F = V2T<float>;
using V2D = V2T<double>;
using V2I = V2T<int>;
using V2U = V2T<uint>;

template <class T1> struct V3T;
using V3 = V3T<float>;
using V3F = V3T<float>;
using V3D = V3T<double>;
using V3I = V3T<int>;
using V3U = V3T<uint>;

using DistanceT = double;
using PointT = V3T<DistanceT>;

template <class T1> struct V4T;
using V4 = V4T<float>;
using V4F = V4T<float>;
using V4D = V4T<double>;
using V4I = V4T<int>;
using V4U = V4T<uint>;

template <class T1, class T2> struct BoundT;
using Bound3 = BoundT<V3, V3>;
using Bound3F = BoundT<V3F, V3>;
using Bound3D = BoundT<V3D, V3>;

template <class T1> struct MatrixT;
using Matrix = MatrixT<float>;
using MatrixF = MatrixT<float>;
using MatrixD = MatrixT<double>;

template <class T1> struct PlaneT;
using Plane = PlaneT<float>;
using PlaneF = PlaneT<float>;
using PlaneD = PlaneT<double>;

template <class T1, class T2> struct RayT;
using Ray = RayT<V3, V3>;
using RayF = RayT<V3F, V3>;
using RayD = RayT<V3D, V3>;

/* Graphics. */
struct Color;
struct Vertex;

using CubeMap = Reference<struct CubeMapT>;
using Font = Reference<struct FontT>;
using Geometry = Reference<struct GeometryT>;
using Mesh = Reference<struct MeshT>;
using Model = Reference<struct ModelT>;
using ParticleSystem = Reference<struct ParticleSystemT>;
using PlateMesh = Reference<struct PlateMeshT>;
using Renderable = Reference<struct RenderableT>;
using RenderPass = Reference<struct RenderPassT>;
using RenderStyle = Reference<struct RenderStyleT>;
using Shader = Reference<struct ShaderT>;
using ShaderInstance = Reference<struct ShaderInstanceT>;
using Viewport = Reference<struct ViewportT>;
using Texture2D = Reference<struct Texture2DT>;
using Texture3D = Reference<struct Texture3DT>;

/* Program. */
struct Program;
struct Timer;

using Lock = Reference<struct LockT>;
using Module = Reference<struct ModuleT>;
using Thread = Reference<struct ThreadT>;
using Window = Reference<struct WindowT>;

/* Reflection. */
struct ConversionType;
struct Field;
struct FieldMapper;
struct FieldType;
struct FunctionDesc;
struct Type;
struct TypeT;

using Function = Reference<struct FunctionT>;
using Package = Reference<struct PackageT>;

/* Misc. */
struct DrawState;
using ResourceMap = Reference<struct ResourceMapT>;

namespace LTE {
  /* Control. */
  using Axis = Reference<struct AxisT>;
  using Button = Reference<struct ButtonT>;
  struct Joystick;

  /* Engine. */
  struct Data;
  struct DataRef;
  struct DataStack;
  struct Diff;
  struct Grammar;
  struct Patch;
  struct String;

  using Expression = Reference<struct ExpressionT>;
  using Job = Reference<struct JobT>;
  using Location = Reference<struct LocationT>;
  using Script = Reference<struct ScriptT>;
  using ScriptFunction = Reference<struct ScriptFunctionT>;
  using ScriptType = Reference<struct ScriptTypeT>;

  /* Function. */
  template <class ValueType, class ArgType> struct Generic;

  using HashT = uint;
  #define HASHT_MAX UINT_MAX

  using GenericBool = Generic<bool, void>;
  using GenericColor = Generic<Color, void>;
  using GenericInt = Generic<int, void>;
  using GenericFloat = Generic<float, void>;
  using GenericVoid = Generic<void, void>;
  using GenericAxis = Generic<Axis, void>;
  using GenericButton = Generic<Button, void>;
  using GenericV2 = Generic<V2, void>;
  using GenericV3 = Generic<V3, void>;
  using GenericV4 = Generic<V4, void>;

  /* Enums. */
  namespace CubeFace {
    enum Enum {
      PositiveX,
      NegativeX,
      PositiveY,
      NegativeY,
      PositiveZ,
      NegativeZ,
      SIZE
    };

    using BitField = unsigned char;
    const unsigned char PositiveXBit = 0x01;
    const unsigned char NegativeXBit = 0x02;
    const unsigned char PositiveYBit = 0x04;
    const unsigned char NegativeYBit = 0x08;
    const unsigned char PositiveZBit = 0x10;
    const unsigned char NegativeZBit = 0x20;
  }

  #define KEY_X                                                                \
    X(A) X(B) X(C) X(D) X(E) X(F) X(G) X(H) X(I) X(J) X(K) X(L) X(M) X(N) X(O) \
    X(P) X(Q) X(R) X(S) X(T) X(U) X(V) X(W) X(X) X(Y) X(Z)                     \
    X(N0) X(N1) X(N2) X(N3) X(N4) X(N5) X(N6) X(N7) X(N8) X(N9)                \
    X(NP0) X(NP1) X(NP2) X(NP3) X(NP4) X(NP5) X(NP6) X(NP7) X(NP8) X(NP9)      \
    X(F1) X(F2) X(F3) X(F4) X(F5) X(F6) X(F7) X(F8) X(F9) X(F10) X(F11) X(F12) \
    X(F13) X(F14) X(F15)                                                       \
    X(Add)                                                                     \
    X(BackSpace)                                                               \
    X(BackSlash)                                                               \
    X(Comma)                                                                   \
    X(Dash)                                                                    \
    X(Delete)                                                                  \
    X(Divide)                                                                  \
    X(Down)                                                                    \
    X(End)                                                                     \
    X(Equal)                                                                   \
    X(Escape)                                                                  \
    X(Home)                                                                    \
    X(Insert)                                                                  \
    X(LBracket)                                                                \
    X(Left)                                                                    \
    X(Menu)                                                                    \
    X(Multiply)                                                                \
    X(PageDown)                                                                \
    X(PageUp)                                                                  \
    X(Pause)                                                                   \
    X(Period)                                                                  \
    X(Quote)                                                                   \
    X(RBracket)                                                                \
    X(Return)                                                                  \
    X(Right)                                                                   \
    X(SemiColon)                                                               \
    X(Slash)                                                                   \
    X(Space)                                                                   \
    X(Subtract)                                                                \
    X(Tab)                                                                     \
    X(Tilde)                                                                   \
    X(Up)                                                                      \
    X(LAlt)                                                                    \
    X(RAlt)                                                                    \
    X(LControl)                                                                \
    X(RControl)                                                                \
    X(LShift)                                                                  \
    X(RShift)                                                                  \
    X(LSystem)                                                                 \
    X(RSystem)

  #define XLIST KEY_X
  #define XTYPE Key
  #include "LTE/XEnum.h"
  #undef XTYPE
  #undef XLIST

  #define MOUSE_BUTTON_X                                                       \
    X(Left)                                                                    \
    X(Middle)                                                                  \
    X(Right)                                                                   \
    X(X1)                                                                      \
    X(X2)

  #define XLIST MOUSE_BUTTON_X
  #define XTYPE MouseButton
  #include "LTE/XEnum.h"
  #undef XTYPE
  #undef XLIST

  #define JOYSTICK_AXIS_X                                                      \
    X(X)                                                                       \
    X(Y)                                                                       \
    X(Z)                                                                       \
    X(R)                                                                       \
    X(U)                                                                       \
    X(V)                                                                       \
    X(PovX)                                                                    \
    X(PovY)

  #define XLIST JOYSTICK_AXIS_X
  #define XTYPE JoystickAxis
  #include "LTE/XEnum.h"
  #undef XTYPE
  #undef XLIST
}

#define offset_of(type, member) ((volatile void const*)offsetof(type, member))

#define MACRO_IDENTITY(x) x

#define DeclareMetadata(T)                                                     \
  LT_API friend Type _Type_Get(T const& t);

#define FIELDS                                                                 \
  static void MapFields([[maybe_unused]] TypeT* type,                           \
    [[maybe_unused]] void* addr,                                               \
    [[maybe_unused]] FieldMapper& m,                                           \
    [[maybe_unused]] void* aux)

#define PRIMITIVE_TYPE_X                                                       \
  X(bool)                                                                      \
  X(char)                                                                      \
  X(signed char)                                                               \
  X(unsigned char)                                                             \
  X(signed short)                                                              \
  X(unsigned short)                                                            \
  X(signed int)                                                                \
  X(unsigned int)                                                              \
  X(signed long)                                                               \
  X(unsigned long)                                                             \
  X(signed long long)                                                          \
  X(unsigned long long)                                                        \
  X(float)                                                                     \
  X(double)

template <class T>
void Swap(T& one, T& two) {
  T temp = one;
  one = two;
  two = temp;
}

#define X(x) template <class StreamT>                                          \
  inline void _ToStream(StreamT& s, x const& t) {                              \
    s << t; }
PRIMITIVE_TYPE_X
X(char const*)
#undef X

template <class StreamT, class T>
void _ToStream(StreamT& s, T* const& t) {
  s << (void*)t;
}

#if 1
template <class StreamT, class T>
void _ToStream([[maybe_unused]] StreamT& s, [[maybe_unused]] T const& t) {
  s << "Unknown Type";
}
#endif

template <class StreamT>
inline void ToStream(StreamT& s, char const* str) {
  s << str;
}

template <class StreamT, class T>
void ToStream(StreamT& s, T const& t) {
  _ToStream(s, t);
}

template <class T>
struct NullBase {
  FIELDS {}
};

struct NoBase {};

#include "StdMath.h"

using namespace LTE;

#endif
