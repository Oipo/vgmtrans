#pragma once
#include "Scanner.h"
#include "BytePattern.h"

enum CompileSnesVersion: uint8_t; // see CompileSnesFormat.h

class CompileSnesScanner:
    public VGMScanner {
 public:
  CompileSnesScanner() {
    USE_EXTENSION(L"spc");
  }
  ~CompileSnesScanner() override = default;

  void Scan(RawFile *file, void *info = 0) override;
  void SearchForCompileSnesFromARAM(RawFile *file);
  void SearchForCompileSnesFromROM(RawFile *file);

 private:
  static BytePattern ptnSetSongListAddress;
};
