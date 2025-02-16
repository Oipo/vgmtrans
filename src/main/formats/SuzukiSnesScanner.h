#pragma once
#include "Scanner.h"
#include "BytePattern.h"

enum SuzukiSnesVersion: uint8_t; // see SuzukiSnesFormat.h

class SuzukiSnesScanner:
    public VGMScanner {
 public:
  SuzukiSnesScanner() {
    USE_EXTENSION(L"spc");
  }
  ~SuzukiSnesScanner() override = default;

  void Scan(RawFile *file, void *info = 0) override;
  void SearchForSuzukiSnesFromARAM(RawFile *file);
  void SearchForSuzukiSnesFromROM(RawFile *file);

 private:
  static BytePattern ptnLoadSongSD3;
  static BytePattern ptnLoadSongBL;
  static BytePattern ptnExecVCmdBL;
  static BytePattern ptnLoadDIR;
  static BytePattern ptnLoadInstr;
};
