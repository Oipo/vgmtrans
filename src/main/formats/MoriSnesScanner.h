#pragma once
#include "Scanner.h"
#include "BytePattern.h"

enum MoriSnesVersion: uint8_t; // see MoriSnesFormat.h

class MoriSnesScanner:
    public VGMScanner {
 public:
  MoriSnesScanner() {
    USE_EXTENSION(L"spc");
  }
  ~MoriSnesScanner() override = default;

  void Scan(RawFile *file, void *info = 0) override;
  void SearchForMoriSnesFromARAM(RawFile *file);
  void SearchForMoriSnesFromROM(RawFile *file);

 private:
  static BytePattern ptnLoadSeq;
  static BytePattern ptnSetDIR;
};
