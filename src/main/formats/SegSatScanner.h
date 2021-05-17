#pragma once
#include "Scanner.h"

class SegSatScanner:
    public VGMScanner {
 public:
  SegSatScanner();
 public:
  ~SegSatScanner() override;

  void Scan(RawFile *file, void *info = 0) override;
};
