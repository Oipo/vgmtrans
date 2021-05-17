#pragma once
#include "Scanner.h"
#include "BytePattern.h"

enum HudsonSnesVersion: uint8_t; // see HudsonSnesFormat.h

class HudsonSnesScanner:
    public VGMScanner {
 public:
  HudsonSnesScanner() {
    USE_EXTENSION(L"spc");
  }
  ~HudsonSnesScanner() override = default;

  void Scan(RawFile *file, void *info = 0) override;
  void SearchForHudsonSnesFromARAM(RawFile *file);
  void SearchForHudsonSnesFromROM(RawFile *file);

 private:
  static BytePattern ptnNoteLenTable;
  static BytePattern ptnGetSeqTableAddrV0;
  static BytePattern ptnGetSeqTableAddrV1V2;
  static BytePattern ptnLoadTrackAddress;
  static BytePattern ptnLoadDIRV0;
};
