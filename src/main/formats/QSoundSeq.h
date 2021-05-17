#pragma once
#include "VGMSeq.h"
#include "SeqTrack.h"
#include "QSoundFormat.h"

enum QSoundVer: uint8_t;

class QSoundSeq:
    public VGMSeq {
 public:
  QSoundSeq(RawFile *file, uint32_t offset, QSoundVer fmt_version, std::wstring &_name);
  ~QSoundSeq() override;

  bool GetHeaderInfo() override;
  bool GetTrackPointers() override;
  bool PostLoad() override;

 public:
  QSoundVer fmt_version;
};


class QSoundTrack
    : public SeqTrack {
 public:
  QSoundTrack(QSoundSeq *_parentSeq, long offset = 0, long length = 0);
  void ResetVars() override;
  bool ReadEvent() override;

 private:
  QSoundVer GetVersion() { return dynamic_cast<QSoundSeq *>(this->parentSeq)->fmt_version; }

  bool bPrevNoteTie;
  uint8_t prevTieNote;
  uint8_t origTieNote;
  uint8_t curDeltaTable;
  uint8_t noteState;
  uint8_t bank;
  uint8_t loop[4];
  uint32_t loopOffset[4];    //used for detecting infinite loops
  uint32_t dur;
};