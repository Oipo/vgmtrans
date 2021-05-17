#pragma once
#include "Scanner.h"

class TriAcePS1Seq;
class TriAcePS1InstrSet;

class TriAcePS1Scanner:
    public VGMScanner {
 public:
  TriAcePS1Scanner();
 public:
  ~TriAcePS1Scanner() override;

  void Scan(RawFile *file, void *info = 0) override;
  void SearchForSLZSeq(RawFile *file);
  void SearchForInstrSet(RawFile *file, std::vector<TriAcePS1InstrSet *> &instrsets);
  TriAcePS1Seq *TriAceSLZDecompress(RawFile *file, uint32_t cfOff);
};
