#ifndef LTE_FunctionBind_h__
#define LTE_FunctionBind_h__

#include "Function.h"
#include "Mutable.h"

#include <tuple>
#include <type_traits>
#include <utility>

/* C++17 polyfill for remove_cvref_t (P0550R2 is C++20, not in
   <type_traits> under C++17 — see CMakeLists.txt:195). Verified: no existing
   polyfill anywhere in src/. Use the unqualified name throughout this file. */
template <class T>
using remove_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;

/* Signature introspection. The primary template handles callables (lambda /
 * functor) via their operator(); explicit specializations below cover free
 * functions and member functions. */
template <class T>
struct BindingTraits : BindingTraits<decltype(&T::operator())> {};

template <class RT, class... ArgT>
struct BindingTraits<RT (*)(ArgT...)> {
  using ReturnType = RT;
  using Args = std::tuple<ArgT...>;
};

template <class RT, class... ArgT>
struct BindingTraits<RT (ArgT...)> {           // function reference form
  using ReturnType = RT;
  using Args = std::tuple<ArgT...>;
};

template <class RT, class C, class... ArgT>     // member fn (non-const)
struct BindingTraits<RT (C::*)(ArgT...)> {
  using ReturnType = RT;
  using Class = C;
  using Args = std::tuple<ArgT...>;
};

template <class RT, class C, class... ArgT>     // member fn (const)
struct BindingTraits<RT (C::*)(ArgT...) const> {
  using ReturnType = RT;
  using Class = C;
  using Args = std::tuple<ArgT...>;
};

/* Owned binding closure; freed by ~FunctionT. */
struct BindingBase {
  virtual ~BindingBase() = default;
};

template <class RT, class ArgsTuple, class FnT>
struct FunctionBinding : BindingBase {
  FnT fn;

  template <class F>
  FunctionBinding(F&& f) : fn(std::forward<F>(f)) {}

  static void Call(void* self, void** in, void* out) {
    static_cast<FunctionBinding*>(self)->Invoke(
      in, out, std::make_index_sequence<std::tuple_size_v<ArgsTuple>>{});
  }

  template <size_t... Is>
  void Invoke(void** in, void* out, std::index_sequence<Is...>) {
    auto args = std::forward_as_tuple(
      *static_cast<remove_cvref_t<
        std::tuple_element_t<Is, ArgsTuple>>*>(in[Is])...);
    if constexpr (std::is_void_v<RT>)
      std::apply(fn, args);
    else
      *static_cast<remove_cvref_t<RT>*>(out) = std::apply(fn, args);
  }
};

/* Fill params[i] with (name, Type_Get<remove_cvref_t<Arg>>()). */
template <class ArgsTuple, size_t... Is, class... NameT>
void FillParams(Parameter* params, std::index_sequence<Is...>,
                NameT const&... names) {
  ((Mutable(params[Is]) = Parameter(
      names, Type_Get<remove_cvref_t<
        std::tuple_element_t<Is, ArgsTuple>>>())), ...);
}

template <class FnT, class... NameT>
Function Function_Bind(String const& name, String const& desc, FnT&& fn,
                       NameT const&... names) {
  using Traits = BindingTraits<std::remove_reference_t<FnT>>;
  using RT = typename Traits::ReturnType;
  using ArgsTuple = typename Traits::Args;
  static constexpr size_t kArity = std::tuple_size_v<ArgsTuple>;
  static_assert(sizeof...(NameT) == kArity,
    "Function_Bind: parameter name count must equal parameter count");

  auto* binding = new FunctionBinding<RT, ArgsTuple, remove_cvref_t<FnT>>(
    std::forward<FnT>(fn));

  Function f = Function_Create(name);
  f->description = desc;
  f->call = &FunctionBinding<RT, ArgsTuple, remove_cvref_t<FnT>>::Call;
  f->binding = binding;
  f->paramCount = kArity;
  f->returnType = Type_Get<remove_cvref_t<RT>>();
  if constexpr (kArity > 0) {
    auto* params = new Parameter[kArity];
    FillParams<ArgsTuple>(params,
      std::make_index_sequence<kArity>{}, names...);
    f->params = params;
  }
  return f;
}

/* Alias registration (static-init; MUST appear after the source binding). */
inline int Function_Alias(char const* source, char const* alias) {
  Function_AddAlias(source, alias);
  return 0;
}

/* Member-function binding. */
template <class RT, class C, class ArgsTuple, class MemT>
struct FunctionMemberBinding : BindingBase {
  MemT mem;                                     // RT (C::*)(Args...)
  template <class M>
  FunctionMemberBinding(M&& m) : mem(std::forward<M>(m)) {}

  static void Call(void* self, void** in, void* out) {
    static_cast<FunctionMemberBinding*>(self)->Invoke(
      in, out, std::make_index_sequence<std::tuple_size_v<ArgsTuple>>{});
  }

  template <size_t... Is>
  void Invoke(void** in, void* out, std::index_sequence<Is...>) {
    C* receiver = static_cast<C*>(in[0]);
    auto args = std::forward_as_tuple(
      *static_cast<remove_cvref_t<
        std::tuple_element_t<Is, ArgsTuple>>*>(in[Is + 1])...);
    if constexpr (std::is_void_v<RT>)
      std::apply(mem, std::tuple_cat(std::tie(receiver), args));
    else
      *static_cast<remove_cvref_t<RT>*>(out) =
        std::apply(mem, std::tuple_cat(std::tie(receiver), args));
  }
};

template <class MemT, class... NameT>
Function Function_Bind_Member(String const& name, String const& desc,
                              MemT mem, NameT const&... names) {
  using Traits = BindingTraits<MemT>;   // member-fn pointer
  using RT = typename Traits::ReturnType;
  using C = typename Traits::Class;
  using ArgsTuple = typename Traits::Args;
  static constexpr size_t kArity = std::tuple_size_v<ArgsTuple>;
  static_assert(sizeof...(NameT) == kArity,
    "Function_Bind_Member: name count must equal member param count");

  auto* binding = new FunctionMemberBinding<RT, C, ArgsTuple, MemT>(mem);

  Function f = Function_Create(name);
  f->description = desc;
  f->call = &FunctionMemberBinding<RT, C, ArgsTuple, MemT>::Call;
  f->binding = binding;
  f->paramCount = kArity + 1;                 // receiver + args
  auto* params = new Parameter[kArity + 1];
  params[0] = Parameter("object", nullptr);   // filled by TypeT::AddFunction
  FillParams<ArgsTuple>(params + 1, std::make_index_sequence<kArity>{}, names...);
  f->params = params;
  f->returnType = Type_Get<remove_cvref_t<RT>>();
  return f;
}

#endif
