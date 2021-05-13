#pragma once
#include "VGMSeqNoTrks.h"

class HeartBeatPS1Seq:
    public VGMSeqNoTrks {
 public:
  HeartBeatPS1Seq(RawFile *file, uint32_t offset, uint32_t length = 0, const std::wstring &_name = L"HeartBeatPS1Seq");
  ~HeartBeatPS1Seq() override;

  bool GetHeaderInfo() override;
  void ResetVars() override;
  bool ReadEvent() override;

 protected:
  uint32_t seqHeaderOffset;
  uint8_t runningStatus;
};
