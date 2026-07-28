#ifndef LTE_Parameter_h__
#define LTE_Parameter_h__

#include "String.h"

struct Parameter {
  using BaseType = NoBase;
  using SelfType = Parameter;

  String name;
  Type type;

  Parameter() = default;

  Parameter(String const& name, Type const& type) :
    name(name),
    type(type)
    {}

  FIELDS {
    MAPFIELD(name)
    MAPFIELD(type)
  }

  DefineMetadataInline(Parameter)
};

#endif
