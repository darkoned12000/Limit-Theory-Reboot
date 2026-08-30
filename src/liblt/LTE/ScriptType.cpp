#include "ScriptType.h"

#include "LTE/Evaluator.h"
#include "LTE/String.h"
#include "LTE/Type.h"

#include <sstream>
#include <stdlib.h>

namespace LTE {
namespace {

  void ScriptType_Construct(TypeT* type, void* buf) {
    ScriptType const& self = type->GetAux().Convert<ScriptType>();
    char* buffer = (char*)buf;
    for (size_t i = 0; i < self->fields.size(); ++i) {
      void* fieldPtr = buffer + self->fields[i].offset;
      self->fields[i].type->Construct(fieldPtr);
      if (self->initializers[i]) {
        /* Legacy interpreter path. */
        self->initializers[i]->Evaluate(fieldPtr);
      } else if (i < self->astInitializers.size() && self->astInitializers[i]) {
        /* New compiler path: evaluate the AST default into the field slot. */
        Evaluator eval;
        Value v = eval.Evaluate(self->astInitializers[i]);
        if (!eval.HasErrors())
          Evaluator::ValueToSlot(v, self->fields[i].type, fieldPtr);
      }
    }
  }

  void ScriptType_Destruct(TypeT* type, void* buf) {
    ScriptType const& self = type->GetAux().Convert<ScriptType>();
    char* buffer = (char*)buf;
    for (size_t i = 0; i < self->fields.size(); ++i)
      self->fields[i].type->Destruct(buffer + self->fields[i].offset);
  }

  void* ScriptType_Allocate(TypeT* type) {
    ScriptType const& self = type->GetAux().Convert<ScriptType>();
    char* buffer = (char*)malloc(self->size);
    ScriptType_Construct(type, buffer);
    return buffer;
  }

  void ScriptType_Deallocate(TypeT* type, void* buf) {
    ScriptType_Destruct(type, buf);
    free(buf);
  }

  void ScriptType_Assign(TypeT* type, void const* src, void* dst) {
    ScriptType const& self = type->GetAux().Convert<ScriptType>();
    char const* srcBuf = (char const*)src;
    char* dstBuf = (char*)dst;
    for (size_t i = 0; i < self->fields.size(); ++i)
      self->fields[i].type->Assign(
        srcBuf + self->fields[i].offset,
        dstBuf + self->fields[i].offset);
  }

  void ScriptType_Map(TypeT* type, void* buf, FieldMapper& m, void* aux) {
    ScriptType const& self = type->GetAux().Convert<ScriptType>();
    char* buffer = (char*)buf;
    for (size_t i = 0; i < self->fields.size(); ++i)
      m(buffer + self->fields[i].offset,
        self->fields[i].name,
        self->fields[i].type,
        aux);
  }

  void ScriptType_ToString(TypeT* type, void* ptr, String* string) {
    ScriptType const& self = type->GetAux().Convert<ScriptType>();
    std::stringstream stream;
    stream << type->name << " @ " << ptr << "\n";

    Vector<String> split;
    for (size_t i = 0; i < self->fields.size(); ++i) {
      stream << "  " << self->fields[i].name << " : ";

      String result = self->fields[i].type->ToString(
        (char*)ptr + self->fields[i].offset);

      String_Split(split, result, '\n');
      if (split.size()) {
        stream << split[0] << '\n';
        for (size_t j = 1; j < split.size(); ++j)
          stream << "  " << split[j] << '\n';
      } else {
        stream << '\n';
      }
    }
    *string = stream.str();
  }

} // namespace

  Type ScriptType_CreateEngineType(ScriptType const& type, size_t alignment) {
    Type hardType = Type_Create(type->name, type->size);
    hardType->GetAux() = type;
    hardType->GetFields() = type->fields;
    hardType->alignment = alignment;
    hardType->allocate = ScriptType_Allocate;
    hardType->assign = ScriptType_Assign;
    hardType->construct = ScriptType_Construct;
    hardType->deallocate = ScriptType_Deallocate;
    hardType->destruct = ScriptType_Destruct;
    hardType->mapper = ScriptType_Map;
    hardType->toString = ScriptType_ToString;
    type->type = hardType;
    return hardType;
  }

} // namespace LTE
