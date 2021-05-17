#pragma once
#include "Scanner.h"

class SquarePS2Scanner:
    public VGMScanner {
 public:
  SquarePS2Scanner();
 public:
  ~SquarePS2Scanner() override;

  void Scan(RawFile *file, void *info = 0) override;
  void SearchForBGMSeq(RawFile *file);
  void SearchForWDSet(RawFile *file);
};

