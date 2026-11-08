#pragma once
#include <cstdint>

// Apple Home InputSource Identifier must be unique and nonzero.
// In v0.7 IDs are ordinal (1-5), stable because inputs are append-only.
namespace bridge {
inline bool effectsInputIdentifierValid(unsigned inputId, unsigned count) {
  return inputId >= 1 && inputId <= count && count <= 5;
}
}
