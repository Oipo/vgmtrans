#pragma once
#include "Scanner.h"
#include "BytePattern.h"

class RareSnesScanner:
    public VGMScanner {
 public:
  RareSnesScanner() {
    USE_EXTENSION(L"spc");
  }
  virtual ~RareSnesScanner() {
  }

  virtual void Scan(RawFile *file, void *info = 0);
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
