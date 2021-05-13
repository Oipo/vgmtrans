#pragma once
#include "VGMSeq.h"
#include "SeqTrack.h"
#include "SquarePS2Format.h"

class BGMSeq:
    public VGMSeq {
 public:
  BGMSeq(RawFile *file, uint32_t offset);
  virtual ~BGMSeq();

  bool GetHeaderInfo() override;
  virtual bool GetTrackPointers();
  virtual uint32_t GetID() { return assocWDID; }

 protected:
  unsigned short seqID;
  unsigned short assocWDID;
};


class BGMTrack
    : public SeqTrack {
 public:
  BGMTrack(BGMSeq *parentSeq, long offset = 0, long length = 0);

  virtual bool ReadEvent();
};