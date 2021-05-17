#pragma once
#include "Scanner.h"
#include "BytePattern.h"

enum HeartBeatSnesVersion: uint8_t; // see HeartBeatSnesFormat.h

class HeartBeatSnesScanner:
    public VGMScanner {
 public:
  HeartBeatSnesScanner() {
    USE_EXTENSION(L"spc");
  }
  ~HeartBeatSnesScanner() override = default;

  void Scan(RawFile *file, void *info = 0) override;
  void SearchForHeartBeatSnesFromARAM(RawFile *file) const;
  void SearchForHeartBeatSnesFromROM(RawFile *file) const;

 private:
  static BytePattern ptnReadSongList;
  static BytePattern ptnSetDIR;
  static BytePattern ptnLoadSRCN;
  static BytePattern ptnSaveSeqHeaderAddress;
};
