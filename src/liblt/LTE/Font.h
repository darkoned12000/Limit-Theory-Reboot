#ifndef LTE_Font_h__
#define LTE_Font_h__

#include "Reference.h"
#include "String.h"

using Font = Reference<struct FontT>;

struct FontT : public RefCounted {
  FontT() = default;
  virtual ~FontT() = default;

  virtual void Draw(
    String const& text,
    V2 const& position,
    float size,
    Color const& color,
    float alpha,
    bool additive) const = 0;

  virtual V2 GetTextSize(String const& text, float size) const = 0;

  /* FontT is an abstract RefCounted base, only ever used through
     Reference<FontT>. Reflected so Type_Get<FontT>() resolves to a real
     (named) type instead of the generic "unknown type" fallback, which
     otherwise left Reference<FontT> as "Reference<unknown type>" (null
     function pointers) and crashed any Data -> Font conversion. */
  DeclareMetadata(FontT)
};

LT_API Font Font_Get(
  String const& path);

#endif
