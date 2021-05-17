#pragma once
#include "VGMSeqNoTrks.h"
#include "SegSatFormat.h"
#include "SegSatScanner.h"

class SegSatSeq:
    public VGMSeqNoTrks {
 public:
  SegSatSeq(RawFile *file, uint32_t offset);
  ~SegSatSeq() override;

  bool GetHeaderInfo() override;
  bool ReadEvent() override;

 public:
  uint8_t headerFlag;
  int remainingEventsInLoop;
  uint32_t loopEndPos;
};