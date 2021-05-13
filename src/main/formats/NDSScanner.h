#pragma once
#include "Scanner.h"

class NDSScanner:
    public VGMScanner {
 public:
  NDSScanner() {
    USE_EXTENSION(L"nds")
    USE_EXTENSION(L"sdat")
  }
  virtual ~NDSScanner() {
  }


  virtual void Scan(RawFile *file, void *info = 0);
  void SearchForSDAT(RawFile *file);
  uint32_t LoadFromSDAT(RawFile *file, uint32_t offset);
};
