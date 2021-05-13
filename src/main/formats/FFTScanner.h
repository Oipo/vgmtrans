#pragma once
#include "Scanner.h"

class FFTScanner:
    public VGMScanner {
 public:
  ~FFTScanner() override = default;

  void Scan(RawFile *file, void *info = 0) override;   //scan "smds" and "wds"
  void SearchForFFTSeq(RawFile *file) const;                //scan "smds"
  void SearchForFFTwds(RawFile *file) const;                //scan "wds"

};



