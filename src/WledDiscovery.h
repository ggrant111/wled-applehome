#pragma once
#include <Arduino.h>
#include "WledClient.h"

class WledDiscovery {
public:
  static constexpr unsigned kMaxCandidates = 20;
  void scan();
  void list() const;
  unsigned count() const { return count_; }
  const WledInfo *at(unsigned index) const {
    return index < count_ ? &candidates_[index] : nullptr;
  }

private:
  WledInfo candidates_[kMaxCandidates];
  unsigned count_ = 0;
  bool exists(const WledInfo &info) const;
};
