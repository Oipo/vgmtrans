#pragma once
#include "Scanner.h"
#include "BytePattern.h"

class RareSnesScanner:
    public VGMScanner {
 public:
  RareSnesScanner() {
    USE_EXTENSION(L"spc");
  }
  ~RareSnesScanner() override = default;

  void Scan(RawFile *file, void *info = 0) override;
  void SearchForRareSnesFromARAM(RawFile *file);
  void SearchForRareSnesFromROM(RawFile *file);

 private:
  static BytePattern ptnLoadDIR;
  static BytePattern ptnReadSRCNTable;
  static BytePattern ptnSongLoadDKC;
  static BytePattern ptnSongLoadDKC2;
  static BytePattern ptnVCmdExecDKC;
  static BytePattern ptnVCmdExecDKC2;
};
