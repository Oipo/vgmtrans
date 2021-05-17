#pragma once
#include "VGMSeq.h"
#include "SeqTrack.h"
#include "TriAcePS1Format.h"

class TriAcePS1ScorePattern;

class TriAcePS1Seq:
    public VGMSeq {
 public:
  struct TrkInfo {
    uint16_t unknown1;
    uint16_t unknown2;
    uint16_t trkOffset;
  };


  TriAcePS1Seq(RawFile *file, uint32_t offset, const std::wstring &_name = std::wstring(L"TriAce Seq"));
  ~TriAcePS1Seq() override;

  bool GetHeaderInfo() override;
  bool GetTrackPointers() override;
  void ResetVars() override;

  VGMHeader *header;
  TrkInfo TrkInfos[32];
  std::vector<TriAcePS1ScorePattern *> aScorePatterns;
  TriAcePS1ScorePattern *curScorePattern;
  std::map<uint32_t, TriAcePS1ScorePattern *> patternMap;
  uint8_t initialTempoBPM;
};

class TriAcePS1ScorePattern
    : public VGMContainerItem {
 public:
  TriAcePS1ScorePattern(TriAcePS1Seq *parentSeq, uint32_t offset)
      : VGMContainerItem(parentSeq, offset, 0, L"Score Pattern") { }
};


class TriAcePS1Track
    : public SeqTrack {
 public:
  TriAcePS1Track(TriAcePS1Seq *_parentSeq, long offset = 0, long length = 0);

  void LoadTrackMainLoop(uint32_t stopOffset, int32_t stopTime) override;
  uint32_t ReadScorePattern(uint32_t offset);
  bool IsOffsetUsed(uint32_t offset) override;
  void AddEvent(SeqEvent *pSeqEvent) override;
  bool ReadEvent() override;

  uint8_t impliedNoteDur;
  uint8_t impliedVelocity;
};