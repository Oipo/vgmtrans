#pragma once
#include "Scanner.h"
#include "BytePattern.h"

class NeverlandSnesScanner:
    public VGMScanner {
 public:
  NeverlandSnesScanner() {
    USE_EXTENSION(L"spc");
  }
  ~NeverlandSnesScanner() override = default;

  void Scan(RawFile *file, void *info = 0) override;
  void SearchForNeverlandSnesFromARAM(RawFile *file);
  void SearchForNeverlandSnesFromROM(RawFile *file);

 private:
  static BytePattern ptnLoadSongSFC;
  static BytePattern ptnLoadSongS2C;
};
