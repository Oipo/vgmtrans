#pragma once
#include "Scanner.h"
#include "BytePattern.h"

class PandoraBoxSnesScanner:
    public VGMScanner {
 public:
  PandoraBoxSnesScanner() {
    USE_EXTENSION(L"spc");
  }
  virtual ~PandoraBoxSnesScanner() {
  }

  virtual void Scan(RawFile *file, void *info = 0);
  void SearchForPandoraBoxSnesFromARAM(RawFile *file);
  void SearchForPandoraBoxSnesFromROM(RawFile *file);

 private:
  static BytePattern ptnLoadSeqKKO;
  static BytePattern ptnLoadSeqTSP;
  static BytePattern ptnSetDIR;
  static BytePattern ptnLoadSRCN;
};
