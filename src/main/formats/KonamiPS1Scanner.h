#pragma once
#include "Scanner.h"

class KonamiPS1Scanner : public VGMScanner {
 public:
  KonamiPS1Scanner() = default;
  ~KonamiPS1Scanner() override = default;

  void Scan(RawFile *file, void *info = 0) override;
};
