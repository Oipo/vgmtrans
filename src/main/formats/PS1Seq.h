#pragma once
#include "VGMSeqNoTrks.h"

class PS1Seq:
    public VGMSeqNoTrks {
 public:
  PS1Seq(RawFile *file, uint32_t offset);
  ~PS1Seq() override;

  bool GetHeaderInfo() override;
  void ResetVars() override;
  bool ReadEvent() override;

 protected:
  uint8_t runningStatus;
};
