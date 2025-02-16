#pragma once
#include "VGMSeq.h"
#include "SeqTrack.h"
#include "SeqEvent.h"
#include "NeverlandSnesFormat.h"

enum NeverlandSnesSeqEventType {
  //start enum at 1 because if map[] look up fails, it returns 0, and we don't want that to get confused with a legit event
  EVENT_UNKNOWN0 = 1,
  EVENT_UNKNOWN1,
  EVENT_UNKNOWN2,
  EVENT_UNKNOWN3,
  EVENT_UNKNOWN4,
};

class NeverlandSnesSeq
    : public VGMSeq {
 public:
  NeverlandSnesSeq(RawFile *file, NeverlandSnesVersion ver, uint32_t seqdataOffset);
  ~NeverlandSnesSeq() override;

  bool GetHeaderInfo() override;
  bool GetTrackPointers() override;
  void ResetVars() override;

  NeverlandSnesVersion version;
  std::map<uint8_t, NeverlandSnesSeqEventType> EventMap;

  uint16_t ConvertToAPUAddress(uint16_t offset);
  uint16_t GetShortAddress(uint32_t offset);

 private:
  void LoadEventMap();
};


class NeverlandSnesTrack
    : public SeqTrack {
 public:
  NeverlandSnesTrack(NeverlandSnesSeq *parentFile, long offset = 0, long length = 0);
  void ResetVars() override;
  bool ReadEvent() override;

  uint16_t ConvertToAPUAddress(uint16_t offset);
  uint16_t GetShortAddress(uint32_t offset);
};
