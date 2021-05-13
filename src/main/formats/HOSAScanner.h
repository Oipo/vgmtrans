#pragma once
#include "Scanner.h"

class HOSASeq;
class HOSAInstrSet;
class PSXSampColl;

class HOSAScanner:
    public VGMScanner {
 public:
  HOSAScanner();
 public:
  ~HOSAScanner() override;

  void Scan(RawFile *file, void *info = 0) override;
  HOSASeq *SearchForHOSASeq(RawFile *file) const;
  HOSAInstrSet *SearchForHOSAInstrSet(RawFile *file, PSXSampColl *sampcoll);
  bool RecursiveRgnCompare(RawFile *file, int i, int sampNum, int numSamples, int numFinds, uint32_t *sampOffsets);
};



