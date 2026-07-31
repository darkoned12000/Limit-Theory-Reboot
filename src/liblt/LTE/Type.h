#ifndef LTE_Type_h__
#define LTE_Type_h__

#include "Common.h"
#include "Mutable.h"
#include "String.h"

#include <typeindex>

#define DECLARE_DEFAULT_REFLECTION(name)                                       \
  LT_API Type _Type_Get(name const& t);

#define MAKE_DEFAULT_REFLECTION(T)                                             \
  Type _Type_Get(T const& t) {                                                 \
    Type& type = Type_GetStorage<T>();                                         \
    if (!type) {                                                               \
      type = Type_Create(#T, sizeof(T));                                       \
      type->alignment = AlignOf<T>();                                          \
      type->allocate = __type_default_allocator<T>;                            \
      type->assign = __type_default_assign<T>;                                 \
      type->construct = __type_default_construct<T>;                           \
      type->deallocate = __type_default_deallocator<T>;                        \
      type->destruct = __type_default_destruct<T>;                             \
      type->toString = __type_default_tostring<T>;                             \
    }                                                                          \
    return type;                                                               \
  }

#define DeclareMetadata(T)                                                     \
  LT_API friend Type _Type_Get(T const& t);

#define DefineMetadata(T)                                                      \
  Type _Type_Get([[maybe_unused]] T const& t) {                                \
    Type& type = Type_GetStorage<T>();                                         \
    if (!type) {                                                               \
      type = Type_Create(#T, sizeof(T));                                       \
      type->alignment = AlignOf<T>();                                          \
      type->allocate = __type_default_allocator<T>;                            \
      type->assign = __type_default_assign<T>;                                 \
      type->base = Type_Get<T::BaseType>();                                    \
      type->construct = __type_default_construct<T>;                           \
      type->deallocate = __type_default_deallocator<T>;                        \
      type->destruct = __type_default_destruct<T>;                             \
      type->mapper = T::MapFields;                                             \
      type->toString = __type_default_tostring<T>;                             \
      FillMetadata(type);                                                      \
    }                                                                          \
    return type;                                                               \
  }

#define DefineMetadataInline(T)                                                \
  friend DefineMetadata(T)

#define AUTOMATIC_REFLECTION_PARAMETRIC1(T, T1T1)                              \
  friend Type _Type_Get([[maybe_unused]] T const& t) {                         \
    Type& type = Type_GetStorage<T>();                                         \
    if (!type) {                                                               \
      Type t1Type = Type_Get<T1T1>();                                          \
      type = Type_Create(                                                      \
        String(#T) + "<" + t1Type->name + ">",                                 \
        sizeof(T));                                                            \
      type->alignment = AlignOf<T>();                                          \
      type->allocate = __type_default_allocator<T>;                            \
      type->assign = __type_default_assign<T>;                                 \
      type->base = Type_Get<T::BaseType>();                                    \
      type->construct = __type_default_construct<T>;                           \
      type->deallocate = __type_default_deallocator<T>;                        \
      type->destruct = __type_default_destruct<T>;                             \
      type->mapper = T::MapFields;                                             \
      type->toString = __type_default_tostring<T>;                             \
      FillMetadata(type);                                                      \
    }                                                                          \
    return type;                                                               \
  }

#define AUTOMATIC_REFLECTION_PARAMETRIC2(T, T1T1, T2T2)                      \
  friend Type _Type_Get([[maybe_unused]] T const& t) {                         \
    Type& type = Type_GetStorage<T>();                                         \
    if (!type) {                                                               \
      Type t1Type = Type_Get<T1T1>();                                          \
      Type t2Type = Type_Get<T2T2>();                                          \
      type = Type_Create(                                                      \
        String(#T) + "<" + t1Type->name + ", " + t2Type->name + ">",           \
        sizeof(T));                                                            \
      type->alignment = AlignOf<T>();                                          \
      type->allocate = __type_default_allocator<T>;                            \
      type->assign = __type_default_assign<T>;                                 \
      type->base = Type_Get<T::BaseType>();                                    \
      type->construct = __type_default_construct<T>;                           \
      type->deallocate = __type_default_deallocator<T>;                        \
      type->destruct = __type_default_destruct<T>;                             \
      type->mapper = T::MapFields;                                             \
      type->toString = __type_default_tostring<T>;                             \
      FillMetadata(type);                                                      \
    }                                                                          \
    return type;                                                               \
  }

#define TypeAlias(SourceType, alias)                                           \
  template <int unused>                                                        \
  int __Register_Typedef_##alias() {                                           \
    static Type type;                                                          \
    if (!type) {                                                               \
      type = Type_Get<SourceType>();                                           \
      Type_AddAlias(type, #alias);                                             \
    }                                                                          \
    return 0;                                                                  \
  }                                                                            \
  volatile static int __Typedef_##alias##_Registration = __Register_Typedef_##alias<0>()

#define MAPFIELD(name)                                                         \
  m(&((SelfType*)addr)->name, #name, Type_Get(((SelfType*)addr)->name), aux);

#define MEMBERFUNCTION(name)                                                   \
  type->AddFunction(name##_GetMetadata<0>());

#define METADATA                                                               \
  static void FillMetadata(Type const& type)

#define REGISTER_TYPE(name)                                                    \
  volatile static Type _##name##_type_registration = Type_Get<name>();

using AllocateFn   = void* (*)(TypeT*);
using AssignFn     = void (*)(TypeT*, void const*, void*);
using ConversionFn = void (*)(TypeT*, void const*, void*);
using CastIntFn    = int64 (*)(TypeT*, void const*);
using CastRealFn   = double (*)(TypeT*, void const*);
using ConstructFn  = void (*)(TypeT*, void*);
using DeallocateFn = void (*)(TypeT*, void*);
using DestructFn   = void (*)(TypeT*, void*);
using MapperFn     = void (*)(TypeT*, void*, FieldMapper&, void*);
using ToStringFn   = void (*)(TypeT*, void*, String*);

template <class T>
size_t AlignOf() {
  return alignof(T);
}

struct TypeT {
  uint refCount;
  String name;
  size_t size;
  size_t alignment;
  TypeT* base;
  size_t GUID;
  bool pointer;

  AllocateFn allocate;
  AssignFn assign;
  CastIntFn castInt;
  CastRealFn castReal;
  ConstructFn construct;
  DeallocateFn deallocate;
  DestructFn destruct;
  MapperFn mapper;
  ToStringFn toString;

  TypeT() : refCount(0) {}

  LT_API ~TypeT();

  LT_API void AddConversion(ConversionType const& cast);
  LT_API void AddDerived(Type const& type);
  LT_API void AddField(Field const& field);
  LT_API void AddFunction(Function const& fn);

  LT_API Data& GetAux();
  LT_API Vector<String>& GetAliases();
  LT_API Vector<ConversionType>& GetConversions();
  LT_API Vector<Type>& GetDerived();
  LT_API Vector<Field>& GetFields();
  LT_API Vector<Function>& GetFunctions();
  LT_API Type& GetPointeeType();

  void* Allocate() {
    return allocate(this);
  }

  void Assign(void const* src, void* dst) {
    assign(this, src, dst);
  }

  int64 CastInt(void const* src) {
    return castInt(this, src);
  }

  double CastReal(void const* src) {
    return castReal(this, src);
  }

  void Construct(void* buffer) {
    construct(this, buffer);
  }

  void Deallocate(void* ptr) {
    deallocate(this, ptr);
  }

  void Destruct(void* buffer) {
    destruct(this, buffer);
  }

  bool IsAbstract() const {
    return allocate == 0;
  }

  bool IsComposite() const {
    return mapper != 0;
  }

  bool IsConcrete() const {
    return allocate != 0;
  }

  bool IsPointer() const {
    return pointer;
  }

  bool IsPrimitive() const {
    return mapper == 0;
  }

  void Map(void* addr, FieldMapper& mp, void* aux) {
    mapper(this, addr, mp, aux);
  }

  bool RefCountDecrement() {
    LTE_ASSERT(refCount > 0);
    return --refCount == 0;
  }

  void RefCountIncrement() {
    refCount++;
  }

  LT_API FieldType FindField(void* base, String const& name);
  LT_API String GetAliasName() const;
  LT_API FieldType GetField(void* base, size_t index);
  LT_API size_t GetFieldCount(void* base);
  LT_API bool HasField(void* base, String const& name);
  LT_API bool IsPseudoPointer() const;
  LT_API String ToString(void* buffer);

  template <class StreamT>
  friend void _ToStream(StreamT& s, TypeT const& t) {
    s << t.name;
    if (t.base)
      s << " (" << t.base->name << ')';
    s << " [" << t.size << ']';
  }
};

LT_API Type Type_Create(String const& name, size_t size);

LT_API void Type_AddAlias(Type const& type, String const& alias);
LT_API Type Type_Find(String const& name);
LT_API Vector<Type> const& Type_GetList();
LT_API void Type_Print(void* base, Type const& type, uint maxDepth);

template <class T>
void Type_Print(T const& t, uint maxDepth = 4) {
  Type_Print((void*)&t, Type_Get(t), maxDepth);
}

struct Type {
  using SelfType = Type;
  TypeT* t;

  Type(TypeT* t = 0) : t(t) { Acquire(); }
  Type(Type const& other) : t(other.t) { Acquire(); }
  ~Type() { Release(); }

  operator bool() { return t != 0; }
  operator bool() const { return t != 0; }
  operator TypeT*() const { return t; }

  Type& operator=(TypeT* t) {
    if (t) Mutable(t)->RefCountIncrement();
    Release();
    this->t = t;
    return *this;
  }

  Type& operator=(Type const& ref) {
    if (this == &ref)
      return *this;
    if (ref.t)
      Mutable(ref.t)->RefCountIncrement();
    Release();
    t = ref.t;
    return *this;
  }

  friend bool operator==(Type const& one, Type const& two) { return one.t == two.t; }
  friend bool operator!=(Type const& one, Type const& two) { return one.t != two.t; }
  friend bool operator==(Type const& r, TypeT* t) { return r.t == t; }
  friend bool operator==(TypeT* t, Type const& r) { return r.t == t; }
  friend bool operator!=(Type const& r, TypeT* t) { return r.t != t; }
  friend bool operator!=(TypeT* t, Type const& r) { return r.t != t; }
  friend bool operator<(Type const& one, Type const& two) { return one.t < two.t; }
  friend bool operator<(Type const& r, TypeT* t) { return r.t < t; }
  friend bool operator<(TypeT* t, Type const& r) { return t < r.t; }

  TypeT* operator->() const {
#ifdef DEBUG_POINTERS
    if (!t) error("Attempt to access null reference");
#endif
    return t;
  }

  TypeT& operator*() const {
#ifdef DEBUG_POINTERS
    if (!t) error("Attempt to access null reference");
#endif
    return *t;
  }

  void Acquire() {
    if (t) Mutable(t)->RefCountIncrement();
  }

  void Release() {
    if (t && Mutable(t)->RefCountDecrement())
      delete t;
  }

  template <class StreamT>
  friend void _ToStream(StreamT& stream, Type const& t) {
    if (!t.t)
      stream << "null";
    else
      ToStream(stream, *t.t);
  }
};

struct ConversionType {
  Type other;
  ConversionFn fn;
};

struct FieldMapper {
  virtual void operator()(
    void* field,
    char const* name,
    Type const& type,
    void* aux) = 0;
};

struct FieldType {
  void* address;
  char const* name;
  Type type;

  FieldType() :
    address(0),
    name(0),
    type(0)
    {}

  FieldType(void* address, char const* name, Type const& type) :
    address(address),
    name(name),
    type(type)
    {}
};

template <class T>
Type Type_Get(T const& t);

/* Produce a T const& without constructing a T, for ADL-based type
   resolution in Type_Get<T>(). The reference is never dereferenced by
   _Type_Get (which only uses the template parameter T); this relies on
   the same implementation-defined, universally-tolerated mechanism as
   the standard offsetof macro.

   NOTE: this is a DELIBERATE, centralized end-state — NOT an oversight.
   The scattered *(T const*)0 literals that previously appeared across
   Vec.h / Map.h / VectorMap.h / XVector.h / Type.h were all removed and
   now funnel through this single helper. We evaluated eliminating it
   entirely by switching the _Type_Get(T const&) friends to _Type_Get(T*)
   and passing a null pointer (which would be fully well-defined), but
   that requires converting ~191 real-instance Type_Get(instance) callers
   (e.g. Type_Get(self->n0) in AutoClass_Generated.h) and every reflection
   macro to pass pointers instead of values. That is a large, risky
   refactor of the serialization/reflection core for zero runtime benefit
   (the dereferenced value is never read), so we intentionally keep this
   one tolerated idiom centralized here. */
template <class T>
T const& Type_Ref() {
  return *static_cast<T const*>(0);
}

template <class T>
Type Type_Get() {
  return Type_Get(Type_Ref<T>());
}

inline void FillMetadata([[maybe_unused]] TypeT* type) {}

/* Cache unification (AGENTS §8d #13). Historically each translation unit had its
 * own `static Type t` here, so the exe and the dll ended up with *different*
 * Type objects for the same C++ type T (launch.cpp:62-66 "major design flaw").
 * That made `Type_Get<T>() == Type_Get<T>()` comparisons fail across the
 * exe/dll boundary and let unreflected abstract types resolve to distinct
 * "unknown type" instances. We now keep a single, dll-owned cache keyed by
 * std::type_index so every TU (exe and dll alike) observes the *same* Type for
 * a given T. The map lives in Type.cpp (compiled into liblt.so) and is reached
 * only through this LT_API helper, so there is exactly one instance.
 *
 * The returned reference is into that shared map: it is null until the
 * corresponding _Type_Get(T const&) friend populates it on the first public
 * Type_Get<T>() call (mirroring the old per-T `if (!type)` lazy pattern). */
LT_API Type& Type_GetStorage(std::type_index);

template <class T>
Type& Type_GetStorage() {
  return Type_GetStorage(typeid(T));
}

template <class T>
void* __type_default_allocator(TypeT*) {
  return (void*)new T;
}

template <class T>
void __type_default_assign([[maybe_unused]] TypeT*, void const* src, void* dst) {
  *(T*)dst = *(T*)src;
}

template <class T>
int64 __type_default_castint([[maybe_unused]] TypeT*, void const* t) {
  return (int64)(*(T*)t);
}

template <class T>
double __type_default_castreal([[maybe_unused]] TypeT*, void const* t) {
  return (double)(*(T*)t);
}

template <class T>
void __type_default_construct([[maybe_unused]] TypeT*, void* buf) {
  new (buf) T;
}

template <class T>
void __type_default_deallocator([[maybe_unused]] TypeT*, void* t) {
  delete (T*)t;
}

template <class T>
void __type_default_destruct([[maybe_unused]] TypeT*, void* buf) {
  ((T*)buf)->~T();
}

template <class T>
void __type_default_tostring([[maybe_unused]] TypeT*, void* buf, String* str) {
  std::stringstream stream;
  ToStream(stream, *(T*)buf);
  *(std::string*)str = stream.str();
}

template <class T>
void __type_map_pointer([[maybe_unused]] TypeT*, void* b, FieldMapper& m, void* aux) {
  T** self = (T**)b;
  m((void*)*self, "value", Type_Get(**self), aux);
}

#if 1
/* Allow unknown types. */
template <class T>
Type _Type_Get([[maybe_unused]] T const& t) {
  Type& type = Type_GetStorage<T>();
  if (!type)
    type = Type_Create("unknown type", sizeof(T));
  return type;
}
#endif

template <class T>
Type _Type_Get([[maybe_unused]] T* const& t) {
  Type& type = Type_GetStorage<T*>();
  if (!type) {
    Type pointeeType = Type_Get<T>();
    type = Type_Create(pointeeType->name + "*", sizeof(T*));
    type->alignment = AlignOf<T*>();
    type->allocate = __type_default_allocator<T*>;
    type->assign = __type_default_assign<T*>;
    type->construct = __type_default_construct<T*>;
    type->deallocate = __type_default_deallocator<T*>;
    type->destruct = __type_default_destruct<T*>;
    type->mapper = __type_map_pointer<T>;
    type->pointer = true;
    type->toString = __type_default_tostring<T*>;
    type->GetPointeeType() = pointeeType;
  }
  return type;
}

inline Type _Type_Get([[maybe_unused]] void* const& t) {
  Type& type = Type_GetStorage<void*>();
  if (!type)
    type = Type_Create("void ptr", sizeof(void*));
  return type;
}

LT_API Type _Type_Get(Type const& t);
LT_API Type _Type_Get(TypeT const& t);

#define X(x) DECLARE_DEFAULT_REFLECTION(x)
PRIMITIVE_TYPE_X
#undef X

template <class T>
Type Type_Get(T const& t) {
  return _Type_Get(t);
}

template <>
inline Type Type_Get<void>() {
  Type& type = Type_GetStorage<void>();
  if (!type)
    type = Type_Create("void", 0);
  return type;
}

/* Cache unification (AGENTS §8d #13). Called once at startup (top of
 * Launcher::Launch, before any script compiles) to force-resolve the
 * primitive/essential types into the single shared type-index cache. Safe to
 * call multiple times; it is idempotent. Defined in Type.cpp. */
LT_API void LTE_Initialize();

#endif
