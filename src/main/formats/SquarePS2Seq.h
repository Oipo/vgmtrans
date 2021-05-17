#pragma once
#include "VGMSeq.h"
#include "SeqTrack.h"
#include "SquarePS2Format.h"

class BGMSeq:
    public VGMSeq {
 public:
  BGMSeq(RawFile *file, uint32_t offset);
  ~BGMSeq() override;

  bool GetHeaderInfo() override;
  bool GetTrackPointers() override;
  optional<uint32_t> GetID() override { return assocWDID; }

 protected:
  unsigned short seqID;
  unsigned short assocWDID;
};


class BGMTrack
    : public SeqTrack {
 public:
  BGMTrack(BGMSeq *_parentSeq, long offset = 0, long length = 0);

  bool ReadEvent() override;

  int8_t vel{DEFAULT_VEL};
  int8_t key{};
};