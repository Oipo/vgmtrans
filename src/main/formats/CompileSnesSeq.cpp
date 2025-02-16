#include "pch.h"
#include "CompileSnesSeq.h"

#include <utility>
#include "ScaleConversion.h"

DECLARE_FORMAT(CompileSnes);

//  **************
//  CompileSnesSeq
//  **************
#define MAX_TRACKS  8
#define SEQ_PPQN    12
#define SEQ_KEYOFS  12

#define COMPILESNES_FLAGS_PORTAMENTO    0x10

// duration table
const uint8_t CompileSnesSeq::noteDurTable[] = {
    0x01, 0x02, 0x03, 0x04, 0x06, 0x08, 0x0c, 0x10,
    0x18, 0x20, 0x30, 0x09, 0x12, 0x1e, 0x24, 0x2a,
};

CompileSnesSeq::CompileSnesSeq(RawFile *file, CompileSnesVersion ver, uint32_t seqdataOffset, std::wstring newName)
    : VGMSeq(CompileSnesFormat::name, file, seqdataOffset), version(ver),
      STATUS_PERCUSSION_NOTE_MIN(0xc0),
      STATUS_PERCUSSION_NOTE_MAX(0xdd),
      STATUS_DURATION_DIRECT(0xde),
      STATUS_DURATION_MIN(0xdf),
      STATUS_DURATION_MAX(0xee) {
  name = std::move(newName);

  bLoadTickByTick = true;
  bAllowDiscontinuousTrackData = true;

  LoadEventMap();
}

CompileSnesSeq::~CompileSnesSeq() = default;

void CompileSnesSeq::ResetVars() {
  VGMSeq::ResetVars();
}

bool CompileSnesSeq::GetHeaderInfo() {
  SetPPQN(SEQ_PPQN);

  VGMHeader *header = AddHeader(dwOffset, 0);

  header->AddSimpleItem(dwOffset, 1, L"Number of Tracks");
  nNumTracks = GetByte(dwOffset);
  if (nNumTracks == 0 || nNumTracks > 8) {
    return false;
  }

  uint32_t curOffset = dwOffset + 1;
  for (uint8_t trackIndex = 0; trackIndex < nNumTracks; trackIndex++) {
    std::wstringstream trackName;
    trackName << L"Track " << (trackIndex + 1);

    VGMHeader *trackHeader = header->AddHeader(curOffset, 14, trackName.str());
    trackHeader->AddSimpleItem(curOffset, 1, L"Channel");
    trackHeader->AddSimpleItem(curOffset + 1, 1, L"Flags");
    trackHeader->AddSimpleItem(curOffset + 2, 1, L"Volume");
    trackHeader->AddSimpleItem(curOffset + 3, 1, L"Volume Envelope");
    trackHeader->AddSimpleItem(curOffset + 4, 1, L"Vibrato");
    trackHeader->AddSimpleItem(curOffset + 5, 1, L"Transpose");
    trackHeader->AddTempo(curOffset + 6, 1);
    trackHeader->AddSimpleItem(curOffset + 7, 1, L"Branch ID (Channel #)");
    trackHeader->AddSimpleItem(curOffset + 8, 2, L"Score Pointer");
    trackHeader->AddSimpleItem(curOffset + 10, 1, L"SRCN");
    trackHeader->AddSimpleItem(curOffset + 11, 1, L"ADSR");
    trackHeader->AddSimpleItem(curOffset + 12, 1, L"Pan");
    trackHeader->AddSimpleItem(curOffset + 13, 1, L"Reserved");
    curOffset += 14;
  }

  return true;        //successful
}


bool CompileSnesSeq::GetTrackPointers() {
  uint32_t curOffset = dwOffset + 1;
  for (uint8_t trackIndex = 0; trackIndex < nNumTracks; trackIndex++) {
    uint16_t ofsTrackStart = GetShort(curOffset + 8);

    CompileSnesTrack *track = new CompileSnesTrack(this, ofsTrackStart);
    track->spcInitialFlags = GetByte(curOffset + 1);
    track->spcInitialVolume = GetByte(curOffset + 2);
    track->spcInitialTranspose = GetByte(curOffset + 5);
    track->spcInitialTempo = GetByte(curOffset + 6);
    track->spcInitialSRCN = GetByte(curOffset + 10);
    track->spcInitialPan = GetByte(curOffset + 12);
    aTracks.push_back(track);

    if (trackIndex == 0) {
      AlwaysWriteInitialTempo(GetTempoInBPM(track->spcInitialTempo));
    }

    curOffset += 14;
  }

  return true;
}

void CompileSnesSeq::LoadEventMap() {
  for (unsigned int statusByte = 0x00; statusByte <= 0x7f; statusByte++) {
    EventMap[statusByte] = EVENT_NOTE;
  }

  EventMap[0x80] = EVENT_GOTO;
  EventMap[0x81] = EVENT_LOOP_END;
  EventMap[0x82] = EVENT_END;
  EventMap[0x83] = EVENT_VIBRATO;
  EventMap[0x84] = EVENT_PORTAMENTO_TIME;
  //EventMap[0x85] = 0;
  //EventMap[0x86] = 0;
  EventMap[0x87] = EVENT_VOLUME;
  EventMap[0x88] = EVENT_VOLUME_ENVELOPE;
  EventMap[0x89] = EVENT_TRANSPOSE;
  EventMap[0x8a] = EVENT_VOLUME_REL;
  EventMap[0x8b] = EVENT_UNKNOWN2;
  EventMap[0x8c] = EVENT_UNKNOWN1; // NOP
  EventMap[0x8d] = EVENT_LOOP_COUNT;
  EventMap[0x8e] = EVENT_UNKNOWN1;
  EventMap[0x8f] = EVENT_UNKNOWN1;
  EventMap[0x90] = EVENT_FLAGS;
  EventMap[0x91] = EVENT_UNKNOWN1;
  EventMap[0x92] = EVENT_UNKNOWN1;
  EventMap[0x93] = EVENT_UNKNOWN2;
  EventMap[0x94] = EVENT_UNKNOWN1;
  // 95 no version differences
  EventMap[0x96] = EVENT_TEMPO;
  EventMap[0x97] = EVENT_TUNING;
  EventMap[0x98] = EVENT_UNKNOWN1;
  EventMap[0x99] = EVENT_UNKNOWN0;
  EventMap[0x9a] = EVENT_CALL;
  EventMap[0x9b] = EVENT_RET;
  //EventMap[0x9c] = 0;
  EventMap[0x9d] = EVENT_UNKNOWN1;
  //EventMap[0x9e] = 0;
  EventMap[0x9f] = EVENT_ADSR;
  EventMap[0xa0] = EVENT_PROGCHANGE;
  EventMap[0xa1] = EVENT_PORTAMENTO_ON;
  EventMap[0xa2] = EVENT_PORTAMENTO_OFF;
  EventMap[0xa3] = EVENT_PANPOT_ENVELOPE;
  EventMap[0xa4] = EVENT_UNKNOWN1; // conditional do (channel match), for delay
  EventMap[0xa5] = EVENT_UNKNOWN3; // conditional jump
  EventMap[0xa6] = EVENT_UNKNOWN1;
  EventMap[0xa7] = EVENT_UNKNOWN1;
  EventMap[0xab] = EVENT_PAN;
  EventMap[0xac] = EVENT_UNKNOWN1;
  EventMap[0xad] = EVENT_LOOP_BREAK;
  EventMap[0xae] = EVENT_UNKNOWN0;
  EventMap[0xaf] = EVENT_UNKNOWN0;

  for (unsigned int statusByte = STATUS_PERCUSSION_NOTE_MIN; statusByte <= STATUS_PERCUSSION_NOTE_MAX; statusByte++) {
    EventMap[statusByte] = EVENT_PERCUSSION_NOTE;
  }

  EventMap[STATUS_DURATION_DIRECT] = EVENT_DURATION_DIRECT;
  for (unsigned int statusByte = STATUS_DURATION_MIN; statusByte <= STATUS_DURATION_MAX; statusByte++) {
    EventMap[statusByte] = EVENT_DURATION;
  }

  switch (version) {
    case COMPILESNES_ALESTE:
      EventMap[0xa4] = EVENT_UNKNOWN1;
      EventMap[0xa5] = EVENT_UNKNOWN1;
      EventMap[0xa6] = EVENT_UNKNOWN1;
    default:
      break;
  }
}

double CompileSnesSeq::GetTempoInBPM(uint8_t tempo) const {
  // cite: <http://www6.atpages.jp/appsouko/work/TAS/doc/fps.html>
  const double SNES_NTSC_FRAMERATE = 39375000.0 / 655171.0;

  unsigned int tempoValue = (tempo == 0) ? 256 : tempo;
  return 60000000.0 / (SEQ_PPQN * (1000000.0 / SNES_NTSC_FRAMERATE)) * (tempoValue / 256.0);
}


//  ****************
//  CompileSnesTrack
//  ****************

CompileSnesTrack::CompileSnesTrack(CompileSnesSeq *parentFile, long offset, long length)
    : SeqTrack(parentFile, offset, length) {
  bDetermineTrackLengthEventByEvent = true;
  bWriteGenericEventAsTextEvent = false;
}

void CompileSnesTrack::ResetVars() {
  SeqTrack::ResetVars();

  cKeyCorrection = SEQ_KEYOFS;

  spcNoteDuration = 1;
  spcFlags = spcInitialFlags;
  spcVolume = spcInitialVolume;
  spcTranspose = spcInitialTranspose;
  spcTempo = spcInitialTempo;
  spcSRCN = spcInitialSRCN;
  spcPan = spcInitialPan;
  memset(repeatCount, 0, sizeof(repeatCount));

  transpose = spcTranspose;
}

void CompileSnesTrack::AddInitialMidiEvents(int trackNum) {
  SeqTrack::AddInitialMidiEvents(trackNum);

  double volumeScale;
  AddProgramChangeNoItem(spcSRCN, true);
  AddVolNoItem(Convert7bitPercentVolValToStdMidiVal(spcVolume / 2));
  AddPanNoItem(Convert7bitLinearPercentPanValToStdMidiVal(static_cast<uint8_t>(spcPan) + 0x80 / 2, &volumeScale));
  AddExpressionNoItem(ConvertPercentAmpToStdMidiVal(volumeScale));
  AddReverbNoItem(0);
}

bool CompileSnesTrack::ReadEvent() {
  CompileSnesSeq *_parentSeq = dynamic_cast<CompileSnesSeq *>(this->parentSeq);

  uint32_t beginOffset = curOffset;
  if (curOffset >= 0x10000) {
    return false;
  }

  uint8_t statusByte = GetByte(curOffset++);
  bool bContinue = true;

  std::wstringstream desc;

  auto eventType = static_cast<CompileSnesSeqEventType>(0);
  auto pEventType = _parentSeq->EventMap.find(statusByte);
  if (pEventType != _parentSeq->EventMap.end()) {
    eventType = pEventType->second;
  }

  switch (eventType) {
    case EVENT_UNKNOWN0:
      desc << L"Event: 0x" << std::hex << std::setfill(L'0') << std::setw(2) << std::uppercase << statusByte;
      AddUnknown(beginOffset, curOffset - beginOffset, L"Unknown Event", desc.str());
      break;

    case EVENT_UNKNOWN1: {
      uint8_t arg1 = GetByte(curOffset++);
      desc << L"Event: 0x" << std::hex << std::setfill(L'0') << std::setw(2) << std::uppercase << statusByte
          << std::dec << std::setfill(L' ') << std::setw(0)
          << L"  Arg1: " << arg1;
      AddUnknown(beginOffset, curOffset - beginOffset, L"Unknown Event", desc.str());
      break;
    }

    case EVENT_UNKNOWN2: {
      uint8_t arg1 = GetByte(curOffset++);
      uint8_t arg2 = GetByte(curOffset++);
      desc << L"Event: 0x" << std::hex << std::setfill(L'0') << std::setw(2) << std::uppercase << statusByte
          << std::dec << std::setfill(L' ') << std::setw(0)
          << L"  Arg1: " << arg1
          << L"  Arg2: " << arg2;
      AddUnknown(beginOffset, curOffset - beginOffset, L"Unknown Event", desc.str());
      break;
    }

    case EVENT_UNKNOWN3: {
      uint8_t arg1 = GetByte(curOffset++);
      uint8_t arg2 = GetByte(curOffset++);
      uint8_t arg3 = GetByte(curOffset++);
      desc << L"Event: 0x" << std::hex << std::setfill(L'0') << std::setw(2) << std::uppercase << statusByte
          << std::dec << std::setfill(L' ') << std::setw(0)
          << L"  Arg1: " << arg1
          << L"  Arg2: " << arg2
          << L"  Arg3: " << arg3;
      AddUnknown(beginOffset, curOffset - beginOffset, L"Unknown Event", desc.str());
      break;
    }

    case EVENT_UNKNOWN4: {
      uint8_t arg1 = GetByte(curOffset++);
      uint8_t arg2 = GetByte(curOffset++);
      uint8_t arg3 = GetByte(curOffset++);
      uint8_t arg4 = GetByte(curOffset++);
      desc << L"Event: 0x" << std::hex << std::setfill(L'0') << std::setw(2) << std::uppercase << statusByte
          << std::dec << std::setfill(L' ') << std::setw(0)
          << L"  Arg1: " << arg1
          << L"  Arg2: " << arg2
          << L"  Arg3: " << arg3
          << L"  Arg4: " << arg4;
      AddUnknown(beginOffset, curOffset - beginOffset, L"Unknown Event", desc.str());
      break;
    }

    case EVENT_UNKNOWN5: {
      uint8_t arg1 = GetByte(curOffset++);
      uint8_t arg2 = GetByte(curOffset++);
      uint8_t arg3 = GetByte(curOffset++);
      uint8_t arg4 = GetByte(curOffset++);
      uint8_t arg5 = GetByte(curOffset++);
      desc << L"Event: 0x" << std::hex << std::setfill(L'0') << std::setw(2) << std::uppercase << statusByte
          << std::dec << std::setfill(L' ') << std::setw(0)
          << L"  Arg1: " << arg1
          << L"  Arg2: " << arg2
          << L"  Arg3: " << arg3
          << L"  Arg4: " << arg4
          << L"  Arg5: " << arg5;
      AddUnknown(beginOffset, curOffset - beginOffset, L"Unknown Event", desc.str());
      break;
    }

    case EVENT_GOTO: {
      uint16_t dest = GetShort(curOffset);
      curOffset += 2;
      desc << L"Destination: $" << std::hex << std::setfill(L'0') << std::setw(4) << std::uppercase << dest;
      uint32_t length = curOffset - beginOffset;

      curOffset = dest;
      if (!IsOffsetUsed(dest)) {
        AddGenericEvent(beginOffset, length, L"Jump", desc.str(), CLR_LOOPFOREVER);
      }
      else {
        bContinue = AddLoopForever(beginOffset, length, L"Jump");
      }
      break;
    }

    case EVENT_LOOP_END: {
      uint8_t repeatNest = GetByte(curOffset++);
      uint16_t dest = GetShort(curOffset);
      curOffset += 2;

      desc << L"Nest Level: " << repeatNest << L"  Destination: $" << std::hex << std::setfill(L'0')
          << std::setw(4) << std::uppercase << dest;
      AddGenericEvent(beginOffset, curOffset - beginOffset, L"Repeat End", desc.str(), CLR_LOOP, ICON_ENDREP);

      repeatCount[repeatNest]--;
      if (repeatCount[repeatNest] != 0) {
        curOffset = dest;
      }
      break;
    }

    case EVENT_END: {
      AddEndOfTrack(beginOffset, curOffset - beginOffset);
      bContinue = false;
      break;
    }

    case EVENT_VIBRATO: {
      uint8_t envelopeIndex = GetByte(curOffset++);
      desc << L"Envelope Index: " << envelopeIndex;
      AddGenericEvent(beginOffset,
                      curOffset - beginOffset,
                      L"Vibrato",
                      desc.str(),
                      CLR_MODULATION,
                      ICON_CONTROL);
      break;
    }

    case EVENT_PORTAMENTO_TIME: {
      uint8_t rate = GetByte(curOffset++);
      desc << L"Rate: " << rate;
      AddGenericEvent(beginOffset,
                      curOffset - beginOffset,
                      L"Portamento Time",
                      desc.str(),
                      CLR_PORTAMENTOTIME,
                      ICON_CONTROL);
      break;
    }

    case EVENT_VOLUME: {
      uint8_t newVolume = GetByte(curOffset++);
      spcVolume = newVolume;
      uint8_t midiVolume = Convert7bitPercentVolValToStdMidiVal(spcVolume / 2);
      AddVol(beginOffset, curOffset - beginOffset, midiVolume);
      break;
    }

    case EVENT_VOLUME_ENVELOPE: {
      uint8_t envelopeIndex = GetByte(curOffset++);
      desc << L"Envelope Index: " << envelopeIndex;
      AddGenericEvent(beginOffset,
                      curOffset - beginOffset,
                      L"Volume Envelope",
                      desc.str(),
                      CLR_VOLUME,
                      ICON_CONTROL);
      break;
    }

    case EVENT_TRANSPOSE: {
      int8_t delta = GetByte(curOffset++);
      spcTranspose += delta;
      AddTranspose(beginOffset, curOffset - beginOffset, spcTranspose, L"Transpose (Relative)");
      break;
    }

    case EVENT_VOLUME_REL: {
      int8_t delta = GetByte(curOffset++);
      spcVolume += delta;
      uint8_t midiVolume = Convert7bitPercentVolValToStdMidiVal(spcVolume / 2);
      AddVol(beginOffset, curOffset - beginOffset, midiVolume, L"Volume (Relative)");
      break;
    }

    case EVENT_NOTE: {
      bool rest = (statusByte == 0x00);

      uint8_t duration;
      bool hasDuration = ReadDurationBytes(curOffset, duration);
      if (hasDuration) {
        spcNoteDuration = duration;
        desc << L"Duration: " << duration;
      }

      if (rest) {
        AddRest(beginOffset, curOffset - beginOffset, spcNoteDuration);
      }
      else {
        uint8_t noteNumber = statusByte - 1;
        AddNoteByDur(beginOffset, curOffset - beginOffset, noteNumber, 100, spcNoteDuration,
                     hasDuration ? L"Note with Duration" : L"Note");
        AddTime(spcNoteDuration);
      }
      break;
    }

    case EVENT_LOOP_COUNT: {
      uint8_t repeatNest = GetByte(curOffset++);
      uint8_t times = GetByte(curOffset++);
      int actualTimes = (times == 0) ? 256 : times;

      desc << L"Nest Level: " << repeatNest << L"  Times: " << actualTimes;
      AddGenericEvent(beginOffset, curOffset - beginOffset, L"Loop Count", desc.str(), CLR_LOOP, ICON_STARTREP);

      repeatCount[repeatNest] = times;
      break;
    }

    case EVENT_FLAGS: {
      uint8_t flags = GetByte(curOffset++);
      spcFlags = flags;
      AddGenericEvent(beginOffset,
                      curOffset - beginOffset,
                      L"Flags",
                      desc.str(),
                      CLR_CHANGESTATE,
                      ICON_CONTROL);
      break;
    }

    case EVENT_TEMPO: {
      uint8_t newTempo = GetByte(curOffset++);
      spcTempo = newTempo;
      AddTempoBPM(beginOffset, curOffset - beginOffset, _parentSeq->GetTempoInBPM(newTempo));
      break;
    }

    case EVENT_TUNING: {
      int16_t newTuning;
      if (_parentSeq->version == COMPILESNES_ALESTE ||
          _parentSeq->version == COMPILESNES_JAKICRUSH) {
        newTuning = GetByte(curOffset++);
      }
      else {
        newTuning = GetShort(curOffset);
        curOffset += 2;
      }

      desc << L"Pitch Register Delta: " << newTuning;
      AddGenericEvent(beginOffset,
                      curOffset - beginOffset,
                      L"Tuning",
                      desc.str(),
                      CLR_CHANGESTATE,
                      ICON_CONTROL);
      break;
    }

    case EVENT_CALL: {
      uint16_t dest = GetShort(curOffset);
      curOffset += 2;
      desc << L"Destination: $" << std::hex << std::setfill(L'0') << std::setw(4) << std::uppercase << dest;

      AddGenericEvent(beginOffset,
                      curOffset - beginOffset,
                      L"Pattern Play",
                      desc.str(),
                      CLR_LOOP,
                      ICON_STARTREP);

      subReturnAddress = curOffset;
      curOffset = dest;
      break;
    }

    case EVENT_RET: {
      AddGenericEvent(beginOffset,
                      curOffset - beginOffset,
                      L"End Pattern",
                      desc.str(),
                      CLR_TRACKEND,
                      ICON_ENDREP);
      curOffset = subReturnAddress;
      break;
    }

    case EVENT_PROGCHANGE: {
      uint8_t newProg = GetByte(curOffset++);
      AddProgramChange(beginOffset, curOffset - beginOffset, newProg, true);
      break;
    }

    case EVENT_ADSR: {
      uint8_t envelopeIndex = GetByte(curOffset++);
      desc << L"Envelope Index: " << envelopeIndex;
      AddGenericEvent(beginOffset, curOffset - beginOffset, L"ADSR", desc.str(), CLR_VOLUME, ICON_CONTROL);
      break;
    }

    case EVENT_PORTAMENTO_ON: {
      spcFlags |= COMPILESNES_FLAGS_PORTAMENTO;
      AddGenericEvent(beginOffset,
                      curOffset - beginOffset,
                      L"Portamento On",
                      desc.str(),
                      CLR_PORTAMENTO,
                      ICON_CONTROL);
      break;
    }

    case EVENT_PORTAMENTO_OFF: {
      spcFlags &= ~COMPILESNES_FLAGS_PORTAMENTO;
      AddGenericEvent(beginOffset,
                      curOffset - beginOffset,
                      L"Portamento Off",
                      desc.str(),
                      CLR_PORTAMENTO,
                      ICON_CONTROL);
      break;
    }

    case EVENT_PANPOT_ENVELOPE: {
      uint8_t envelopeIndex = GetByte(curOffset++);
      desc << L"Envelope Index: " << envelopeIndex;
      AddGenericEvent(beginOffset,
                      curOffset - beginOffset,
                      L"Panpot Envelope",
                      desc.str(),
                      CLR_PAN,
                      ICON_CONTROL);
      break;
    }

    case EVENT_PAN: {
      int8_t newPan = GetByte(curOffset++);
      spcPan = newPan;

      double volumeScale;
      uint8_t midiPan = Convert7bitLinearPercentPanValToStdMidiVal(static_cast<uint8_t> (newPan) + 0x80 / 2, &volumeScale);
      AddExpressionNoItem(ConvertPercentAmpToStdMidiVal(volumeScale));
      AddPan(beginOffset, curOffset - beginOffset, midiPan);
      break;
    }

    case EVENT_LOOP_BREAK: {
      uint8_t repeatNest = GetByte(curOffset++);
      uint16_t dest = GetShort(curOffset);
      curOffset += 2;

      desc << L"Nest Level: " << repeatNest << L"  Destination: $" << std::hex << std::setfill(L'0')
          << std::setw(4) << std::uppercase << dest;
      AddGenericEvent(beginOffset, curOffset - beginOffset, L"Repeat Break", desc.str(), CLR_LOOP, ICON_ENDREP);

      repeatCount[repeatNest]--;
      if (repeatCount[repeatNest] == 0) {
        curOffset = dest;
      }
      break;
    }

    case EVENT_DURATION_DIRECT: {
      uint8_t duration;
      if (!ReadDurationBytes(curOffset, duration)) {
        // address out of range
        return false;
      }
      spcNoteDuration = duration;

      desc << L"Duration: " << duration;
      AddGenericEvent(beginOffset,
                      curOffset - beginOffset,
                      L"Duration (Direct)",
                      desc.str(),
                      CLR_DURNOTE,
                      ICON_CONTROL);
      break;
    }

    case EVENT_DURATION: {
      uint8_t duration;
      if (!ReadDurationBytes(curOffset, duration)) {
        // address out of range
        return false;
      }
      spcNoteDuration = duration;

      desc << L"Duration: " << duration;
      AddGenericEvent(beginOffset, curOffset - beginOffset, L"Duration", desc.str(), CLR_DURNOTE, ICON_CONTROL);
      break;
    }

    case EVENT_PERCUSSION_NOTE: {
      uint8_t duration;
      bool hasDuration = ReadDurationBytes(curOffset, duration);
      if (hasDuration) {
        spcNoteDuration = duration;
        desc << L"Duration: " << duration;
      }

      uint8_t percNoteNumber = statusByte - _parentSeq->STATUS_PERCUSSION_NOTE_MIN;
      AddPercNoteByDur(beginOffset, curOffset - beginOffset, percNoteNumber, 100, spcNoteDuration,
                       hasDuration ? L"Percussion Note with Duration" : L"Percussion Note");
      AddTime(spcNoteDuration);
      break;
    }

    default:
      desc << L"Event: 0x" << std::hex << std::setfill(L'0') << std::setw(2) << std::uppercase << statusByte;
      AddUnknown(beginOffset, curOffset - beginOffset, L"Unknown Event", desc.str());
      pRoot->AddLogItem(new LogItem((std::wstring(L"Unknown Event - ") + desc.str()).c_str(),
                                    LOG_LEVEL_ERR,
                                    L"CompileSnesSeq"));
      bContinue = false;
      break;
  }

  //wostringstream ssTrace;
  //ssTrace << L"" << std::hex << std::setfill(L'0') << std::setw(8) << std::uppercase << beginOffset << L": " << std::setw(2) <<statusByte  << L" -> " << std::setw(8) << curOffset << std::endl;
  //OutputDebugString(ssTrace.str());

  return bContinue;
}

bool CompileSnesTrack::ReadDurationBytes(uint32_t &offset, uint8_t &duration) {
  CompileSnesSeq *_parentSeq = dynamic_cast<CompileSnesSeq *>(this->parentSeq);

  uint32_t _curOffset = offset;
  bool durationDispatched = false;
  while (_curOffset < 0x10000) {
    uint8_t statusByte = GetByte(_curOffset++);

    if (statusByte == _parentSeq->STATUS_DURATION_DIRECT) {
      if (_curOffset >= 0x10000) {
        break;
      }

      duration = GetByte(_curOffset++);
      offset = _curOffset;
      durationDispatched = true;
    }
    else if (statusByte >= _parentSeq->STATUS_DURATION_MIN && statusByte <= _parentSeq->STATUS_DURATION_MAX) {
      duration = CompileSnesSeq::noteDurTable[statusByte - _parentSeq->STATUS_DURATION_MIN];
      offset = _curOffset;
      durationDispatched = true;
    }
    else {
      break;
    }
  }
  return durationDispatched;
}
