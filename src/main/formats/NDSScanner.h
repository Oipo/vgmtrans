#pragma once
#include "Scanner.h"

class NDSScanner:
    public VGMScanner {
 public:
  NDSScanner() {
    USE_EXTENSION(L"nds")
    USE_EXTENSION(L"sdat")
  }
  ~NDSScanner() override = default;


  void Scan(RawFile *file, void *info = 0) override;
  void SearchForSDAT(RawFile *file);
  uint32_t LoadFromSDAT(RawFile *file, uint32_t offset);
};
