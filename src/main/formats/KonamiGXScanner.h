#pragma once
#include "Scanner.h"

class KonamiGXScanner:
    public VGMScanner {
 public:
  void Scan(RawFile *file, void *info = 0) override;
  void LoadSeqTable(RawFile *file, uint32_t offset);
};



