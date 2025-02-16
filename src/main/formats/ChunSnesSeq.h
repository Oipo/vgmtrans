#pragma once
#include "VGMSeq.h"
#include "SeqTrack.h"
#include "ChunSnesFormat.h"

#define CHUNSNES_SUBLEVEL_MAX   3

enum ChunSnesSeqEventType {
  EVENT_UNKNOWN0 =
  1, //start enum at 1 because if map[] look up fails, it returns 0, and we don't want that to get confused with a legit event
  EVENT_UNKNOWN1,
  EVENT_UNKNOWN2,
  EVENT_UNKNOWN3,
  EVENT_UNKNOWN4,
  EVENT_NOP,
  EVENT_NOTE,
  EVENT_DURATION_FROM_TABLE,
  EVENT_LOOP_BREAK_ALT,
  EVENT_LOOP_AGAIN_ALT,
  EVENT_ADSR_RELEASE_SR,
  EVENT_ADSR_AND_RELEASE_SR,
  EVENT_SURROUND,
  EVENT_CONDITIONAL_JUMP,
  EVENT_INC_COUNTER,
  EVENT_PITCH_ENVELOPE,
  EVENT_NOISE_ON,
  EVENT_NOISE_OFF,
  EVENT_MASTER_VOLUME_FADE,
  EVENT_EXPRESSION_FADE,
  EVENT_PAN_FADE,
  EVENT_TUNING,
  EVENT_FULL_VOLUME_FADE,
  EVENT_GOTO,
  EVENT_TEMPO,
  EVENT_DURATION_RATE,
  EVENT_VOLUME,
  EVENT_PAN,
  EVENT_ADSR,
  EVENT_PROGCHANGE,
  EVENT_SYNC_NOTE_LEN_ON,
  EVENT_SYNC_NOTE_LEN_OFF,
  EVENT_LOOP_AGAIN,
  EVENT_LOOP_UNTIL,
  EVENT_EXPRESSION,
  EVENT_CALL,
  EVENT_RET,
  EVENT_TRANSPOSE,
  EVENT_PITCH_SLIDE,
  EVENT_ECHO_ON,
  EVENT_ECHO_OFF,
  EVENT_LOAD_PRESET,
  EVENT_END,
};

enum ChunSnesSeqPresetType {
  PRESET_CONDITION =
  1, //start enum at 1 because if map[] look up fails, it returns 0, and we don't want that to get confused with a legit event
};

class ChunSnesSeq
    : public VGMSeq {
 public:
  ChunSnesSeq(RawFile *file,
              ChunSnesVersion ver,
              ChunSnesMinorVersion minorVer,
              uint32_t seqdataOffset,
              std::wstring newName = L"Chun SNES Seq");
  ~ChunSnesSeq() override;

  [[nodiscard]] bool GetHeaderInfo() override;
  [[nodiscard]] bool GetTrackPointers() override;
  void ResetVars() override;

  [[nodiscard]] double GetTempoInBPM(uint8_t tempo) const;

  ChunSnesVersion version;
  ChunSnesMinorVersion minorVersion;
  std::map<uint8_t, ChunSnesSeqEventType> EventMap;
  std::map<uint8_t, ChunSnesSeqPresetType> PresetMap;

  uint8_t initialTempo;
  uint8_t conditionVar;

 private:
  void LoadEventMap();
};


class ChunSnesTrack
    : public SeqTrack {
 public:
  ChunSnesTrack(ChunSnesSeq *parentFile, long offset = 0, long length = 0);
  void ResetVars() override;
  [[nodiscard]] bool ReadEvent() override;

  void SyncNoteLengthWithPriorTrack();
  [[nodiscard]] uint8_t Multiply8bit(uint8_t multiplicand, uint8_t multiplier) const;
  void GetVolumeBalance(int8_t pan, double &volumeLeft, double &volumeRight) const;
  [[nodiscard]] int8_t CalcPanValue(int8_t pan, double &volumeScale) const;
  [[nodiscard]] double CalcTuningValue(int8_t tuning) const;

  uint8_t index;
  int8_t prevNoteKey;
  bool prevNoteSlurred;

  uint8_t noteLength;
  uint8_t noteDurationRate;
  bool syncNoteLen;

  uint8_t loopCount;
  uint8_t loopCountAlt;
  uint8_t subNestLevel;
  uint16_t subReturnAddr[CHUNSNES_SUBLEVEL_MAX];
};
