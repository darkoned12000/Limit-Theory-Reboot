#ifndef Audio_Common_h__
#define Audio_Common_h__

#include "LTE/Common.h"

namespace Audio {
  struct GeneratorT;
  struct Note;
  struct SignalT;

  using Generator = Reference<GeneratorT>;
  using Signal = Reference<SignalT>;
}

#endif
