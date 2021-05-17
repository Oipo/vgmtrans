#pragma once
#include "VGMSeq.h"
#include "SeqTrack.h"
#include "NDSFormat.h"

class NDSSeq:
    public VGMSeq {
 public:
  NDSSeq(RawFile *file, uint32_t offset, uint32_t length = 0, std::wstring _name = L"NDSSeq");

  bool GetHeaderInfo() override;
  bool GetTrackPointers() override;

};


class NDSTrack
    : public SeqTrack {
 public:
  NDSTrack(NDSSeq *parentFile, uint32_t offset = 0, uint32_t length = 0);
  void ResetVars() override;
  bool ReadEvent() override;

  uint8_t jumpCount;
  uint32_t loopReturnOffset;
  uint32_t dur;
  bool hasLoopReturnOffset;
  bool noteWithDelta;
};