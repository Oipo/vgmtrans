#pragma once
#include "Scanner.h"

class SquarePS2Scanner:
    public VGMScanner {
 public:
  SquarePS2Scanner();
 public:
  ~SquarePS2Scanner();

  virtual void Scan(RawFile *file, void *info = 0);
  void SearchForBGMSeq(RawFile *file);
  void SearchForWDSet(RawFile *file);
};

