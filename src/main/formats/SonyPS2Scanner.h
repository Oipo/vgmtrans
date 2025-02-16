#pragma once
#include "Scanner.h"

class SonyPS2Scanner:
    public VGMScanner {
 public:
  SonyPS2Scanner() {
    USE_EXTENSION(L"sq")
    USE_EXTENSION(L"hd")
    USE_EXTENSION(L"bd")
  }
 public:
  ~SonyPS2Scanner() override = default;

  void Scan(RawFile *file, void *info = 0) override;
  void SearchForSeq(RawFile *file);
  void SearchForInstrSet(RawFile *file);
  void SearchForSampColl(RawFile *file);
};
