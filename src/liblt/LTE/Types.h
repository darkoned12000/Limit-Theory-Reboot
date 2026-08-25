#ifndef LTE_Types_h__
#define LTE_Types_h__

#include "Common.h"

LT_API Type Type_Array(Type const& elemType);
LT_API Type Type_Pointer(Type const& elemType);

/* Instance helpers for engine-created array types (Type_Array). An array
   value is a heap-allocated container; registers hold the pointer returned
   by Type_ArrayAlloc. Used by the LTSL array-literal expression. */
LT_API void* Type_ArrayAlloc(Type const& arrayType);
LT_API void Type_ArrayAppend(void* array, void const* element);
LT_API size_t Type_ArraySize(void* array);
LT_API void* Type_ArrayGet(void* array, size_t index);

#endif
