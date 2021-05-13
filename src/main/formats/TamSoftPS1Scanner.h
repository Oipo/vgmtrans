#pragma once
#include "Scanner.h"

class TamSoftPS1Scanner:
    public VGMScanner {
 public:
  TamSoftPS1Scanner() {
    USE_EXTENSION(L"tsq");
    USE_EXTENSION(L"tvb");
  }

  virtual ~TamSoftPS1Scanner() {
  }

  virtual void Scan(RawFile *file, void *info = 0);
};
