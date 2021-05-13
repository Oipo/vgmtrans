#pragma once
#include "VGMSeq.h"
#include "SeqTrack.h"
#include "KonamiGXFormat.h"

class KonamiGXSeq:
    public VGMSeq {
 public:
  KonamiGXSeq(RawFile *file, uint32_t offset);
  ~KonamiGXSeq() override;

  bool GetHeaderInfo() override;
  bool GetTrackPointers() override;
  //bool LoadTracks();

 protected:

};


class KonamiGXTrack
    : public SeqTrack {
 public:
  KonamiGXTrack(KonamiGXSeq *_parentSeq, long offset = 0, long length = 0);

  bool ReadEvent() override;

 private:
  bool bInJump;
  uint8_t prevDelta;
  uint8_t prevDur;
  uint32_t jump_return_offset;
};