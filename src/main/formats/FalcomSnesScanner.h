#pragma once
#include "Scanner.h"
#include "BytePattern.h"

class FalcomSnesScanner:
    public VGMScanner {
 public:
  FalcomSnesScanner() {
    USE_EXTENSION(L"spc");
  }
  ~FalcomSnesScanner() override = default;

  void Scan(RawFile *file, void *info = 0) override;
  void SearchForFalcomSnesFromARAM(RawFile *file);
  void SearchForFalcomSnesFromROM(RawFile *file);

 private:
  static BytePattern ptnLoadSeq;
  static BytePattern ptnSetDIR;
  static BytePattern ptnLoadInstr;
};
