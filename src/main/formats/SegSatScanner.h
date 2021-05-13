#pragma once
#include "Scanner.h"

class SegSatScanner:
    public VGMScanner {
 public:
  SegSatScanner();
 public:
  virtual ~SegSatScanner();

  virtual void Scan(RawFile *file, void *info = 0);
};
