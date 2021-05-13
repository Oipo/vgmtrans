#pragma once
#include "VGMSeqNoTrks.h"

class PS1Seq:
    public VGMSeqNoTrks {
 public:
  PS1Seq(RawFile *file, uint32_t offset);
  virtual ~PS1Seq();

  bool GetHeaderInfo() override;
  virtual void ResetVars();
  virtual bool ReadEvent();

 protected:
  uint8_t runningStatus;
};
