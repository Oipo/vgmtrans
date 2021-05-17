#pragma once
#include "Scanner.h"
#include "BytePattern.h"

class GraphResSnesScanner:
    public VGMScanner {
 public:
  GraphResSnesScanner() {
    USE_EXTENSION(L"spc");
  }
  ~GraphResSnesScanner() override = default;

  void Scan(RawFile *file, void *info = 0) override;
  void SearchForGraphResSnesFromARAM(RawFile *file);
  void SearchForGraphResSnesFromROM(RawFile *file);

 private:
  std::map<uint8_t, uint8_t> GetInitDspRegMap(RawFile *file);

  static BytePattern ptnLoadSeq;
  static BytePattern ptnDspRegInit;
};
