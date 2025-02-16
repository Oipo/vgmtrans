#include "pch.h"
#include "RareSnesSeq.h"

#include <utility>
#include "ScaleConversion.h"

using namespace std;

DECLARE_FORMAT(RareSnes);

//  **********
//  RareSnesSeq
//  **********
#define MAX_TRACKS  8
#define SEQ_PPQN    32
#define SEQ_KEYOFS  36

const uint16_t RareSnesSeq::NOTE_PITCH_TABLE[128] = {
    0x0000, 0x0040, 0x0044, 0x0048, 0x004c, 0x0051, 0x0055, 0x005b,
    0x0060, 0x0066, 0x006c, 0x0072, 0x0079, 0x0080, 0x0088, 0x0090,
    0x0098, 0x00a1, 0x00ab, 0x00b5, 0x00c0, 0x00cb, 0x00d7, 0x00e4,
    0x00f2, 0x0100, 0x010f, 0x011f, 0x0130, 0x0143, 0x0156, 0x016a,
    0x0180, 0x0196, 0x01af, 0x01c8, 0x01e3, 0x0200, 0x021e, 0x023f,
    0x0261, 0x0285, 0x02ab, 0x02d4, 0x02ff, 0x032d, 0x035d, 0x0390,
    0x03c7, 0x0400, 0x043d, 0x047d, 0x04c2, 0x050a, 0x0557, 0x05a8,
    0x05fe, 0x065a, 0x06ba, 0x0721, 0x078d, 0x0800, 0x087a, 0x08fb,
    0x0984, 0x0a14, 0x0aae, 0x0b50, 0x0bfd, 0x0cb3, 0x0d74, 0x0e41,
    0x0f1a, 0x1000, 0x10f4, 0x11f6, 0x1307, 0x1429, 0x155c, 0x16a1,
    0x17f9, 0x1966, 0x1ae9, 0x1c82, 0x1e34, 0x2000, 0x21e7, 0x23eb,
    0x260e, 0x2851, 0x2ab7, 0x2d41, 0x2ff2, 0x32cc, 0x35d1, 0x3904,
    0x3c68, 0x3fff, 0x3fff, 0x3fff, 0x3fff, 0x3fff, 0x3fff, 0x3fff,
    0x3fff, 0x3fff, 0x3fff, 0x3fff, 0x3fff, 0x3fff, 0x3fff, 0x3fff,
    0x3fff, 0x3fff, 0x3fff, 0x3fff, 0x3fff, 0x3fff, 0x3fff, 0x3fff,
    0x3fff, 0x3fff, 0x3fff, 0x3fff, 0x3fff, 0x3fff, 0x3fff, 0x3fff
};

RareSnesSeq::RareSnesSeq(RawFile *file, RareSnesVersion ver, uint32_t seqdataOffset, wstring newName)
    : VGMSeq(RareSnesFormat::name, file, seqdataOffset), version(ver) {
  name = std::move(newName);

  bLoadTickByTick = true;
  bAllowDiscontinuousTrackData = true;

  UseReverb();
  AlwaysWriteInitialReverb(0);

  LoadEventMap();
}

RareSnesSeq::~RareSnesSeq() = default;

void RareSnesSeq::ResetVars() {
  VGMSeq::ResetVars();

  midiReverb = 40;
  switch (version) {
    case RARESNES_DKC:
      timerFreq = 0x3c;
      break;
    default:
      timerFreq = 0x64;
      break;
  }
  tempo = initialTempo;
  tempoBPM = GetTempoInBPM(tempo, timerFreq);
  AlwaysWriteInitialTempo(tempoBPM);
}

bool RareSnesSeq::GetHeaderInfo() {
  SetPPQN(SEQ_PPQN);

  VGMHeader *seqHeader = AddHeader(dwOffset, MAX_TRACKS * 2 + 2, L"Sequence Header");
  uint32_t curHeaderOffset = dwOffset;
  for (int i = 0; i < MAX_TRACKS; i++) {
    uint16_t trkOff = GetShort(curHeaderOffset);
    seqHeader->AddPointer(curHeaderOffset, 2, trkOff, (trkOff != 0), L"Track Pointer");
    curHeaderOffset += 2;
  }
  initialTempo = GetByte(curHeaderOffset);
  seqHeader->AddTempo(curHeaderOffset++, 1, L"Tempo");
  seqHeader->AddUnknownItem(curHeaderOffset++, 1);

  return true;        //successful
}


bool RareSnesSeq::GetTrackPointers() {
  for (int i = 0; i < MAX_TRACKS; i++) {
    uint16_t trkOff = GetShort(dwOffset + i * 2);
    if (trkOff != 0)
      aTracks.push_back(new RareSnesTrack(this, trkOff));
  }
  return true;
}

void RareSnesSeq::LoadEventMap() {
  // common events
  EventMap[0x00] = EVENT_END;
  EventMap[0x01] = EVENT_PROGCHANGE;
  EventMap[0x02] = EVENT_VOLLR;
  EventMap[0x03] = EVENT_GOTO;
  EventMap[0x04] = EVENT_CALLNTIMES;
  EventMap[0x05] = EVENT_RET;
  EventMap[0x06] = EVENT_DEFDURON;
  EventMap[0x07] = EVENT_DEFDUROFF;
  EventMap[0x08] = EVENT_PITCHSLIDEUP;
  EventMap[0x09] = EVENT_PITCHSLIDEDOWN;
  EventMap[0x0a] = EVENT_PITCHSLIDEOFF;
  EventMap[0x0b] = EVENT_TEMPO;
  EventMap[0x0c] = EVENT_TEMPOADD;
  EventMap[0x0d] = EVENT_VIBRATOSHORT;
  EventMap[0x0e] = EVENT_VIBRATOOFF;
  EventMap[0x0f] = EVENT_VIBRATO;
  EventMap[0x10] = EVENT_ADSR;
  EventMap[0x11] = EVENT_MASTVOLLR;
  EventMap[0x12] = EVENT_TUNING;
  EventMap[0x13] = EVENT_TRANSPABS;
  EventMap[0x14] = EVENT_TRANSPREL;
  EventMap[0x15] = EVENT_ECHOPARAM;
  EventMap[0x16] = EVENT_ECHOON;
  EventMap[0x17] = EVENT_ECHOOFF;
  EventMap[0x18] = EVENT_ECHOFIR;
  EventMap[0x19] = EVENT_NOISECLK;
  EventMap[0x1a] = EVENT_NOISEON;
  EventMap[0x1b] = EVENT_NOISEOFF;
  EventMap[0x1c] = EVENT_SETALTNOTE1;
  EventMap[0x1d] = EVENT_SETALTNOTE2;
  EventMap[0x26] = EVENT_PITCHSLIDEDOWNSHORT;
  EventMap[0x27] = EVENT_PITCHSLIDEUPSHORT;
  EventMap[0x2b] = EVENT_LONGDURON;
  EventMap[0x2c] = EVENT_LONGDUROFF;

  switch (version) {
    case RARESNES_DKC:
      EventMap[0x1c] = EVENT_SETVOLADSRPRESET1;
      EventMap[0x1d] = EVENT_SETVOLADSRPRESET2;
      EventMap[0x1e] = EVENT_SETVOLADSRPRESET3;
      EventMap[0x1f] = EVENT_SETVOLADSRPRESET4;
      EventMap[0x20] = EVENT_SETVOLADSRPRESET5;
      EventMap[0x21] = EVENT_GETVOLADSRPRESET1;
      EventMap[0x22] = EVENT_GETVOLADSRPRESET2;
      EventMap[0x23] = EVENT_GETVOLADSRPRESET3;
      EventMap[0x24] = EVENT_GETVOLADSRPRESET4;
      EventMap[0x25] = EVENT_GETVOLADSRPRESET5;
      EventMap[0x28] = EVENT_PROGCHANGEVOL;
      EventMap[0x29] = EVENT_UNKNOWN1;
      EventMap[0x2a] = EVENT_TIMERFREQ;
      EventMap[0x2d] = EVENT_CONDJUMP;
      EventMap[0x2e] = EVENT_SETCONDJUMPPARAM;
      EventMap[0x2f] = EVENT_TREMOLO;
      EventMap[0x30] = EVENT_TREMOLOOFF;
      break;

    case RARESNES_KI:
      //removed common events
      EventMap.erase(0x0c);
      EventMap.erase(0x0d);
      EventMap.erase(0x11);
      EventMap.erase(0x15);
      EventMap.erase(0x18);
      EventMap.erase(0x19);
      EventMap.erase(0x1a);
      EventMap.erase(0x1b);
      EventMap.erase(0x1c);
      EventMap.erase(0x1d);

      EventMap[0x1e] = EVENT_VOLCENTER;
      EventMap[0x1f] = EVENT_CALLONCE;
      EventMap[0x20] = EVENT_RESETADSR;
      EventMap[0x21] = EVENT_RESETADSRSOFT;
      EventMap[0x22] = EVENT_VOICEPARAMSHORT;
      EventMap[0x23] = EVENT_ECHODELAY;
      //EventMap[0x24] = null;
      //EventMap[0x25] = null;
      //EventMap[0x28] = null;
      //EventMap[0x29] = null;
      //EventMap[0x2a] = null;
      //EventMap[0x2d] = null;
      //EventMap[0x2e] = null;
      //EventMap[0x2f] = null;
      //EventMap[0x30] = null;
      break;

    case RARESNES_DKC2:
      //removed common events
      EventMap.erase(0x11);

      EventMap[0x1e] = EVENT_SETVOLPRESETS;
      EventMap[0x1f] = EVENT_ECHODELAY;
      EventMap[0x20] = EVENT_GETVOLPRESET1;
      EventMap[0x21] = EVENT_CALLONCE;
      EventMap[0x22] = EVENT_VOICEPARAM;
      EventMap[0x23] = EVENT_VOLCENTER;
      EventMap[0x24] = EVENT_MASTVOL;
      //EventMap[0x25] = null;
      //EventMap[0x28] = null;
      //EventMap[0x29] = null;
      //EventMap[0x2a] = null;
      //EventMap[0x2d] = null;
      //EventMap[0x2e] = null;
      //EventMap[0x2f] = null;
      EventMap[0x30] = EVENT_ECHOOFF; // duplicated
      EventMap[0x31] = EVENT_GETVOLPRESET2;
      EventMap[0x32] = EVENT_ECHOOFF; // duplicated
      break;

    case RARESNES_WNRN:
      //removed common events
      EventMap.erase(0x19);
      EventMap.erase(0x1a);
      EventMap.erase(0x1b);

      //EventMap[0x1e] = null;
      //EventMap[0x1f] = null;
      EventMap[0x20] = EVENT_MASTVOL;
      EventMap[0x21] = EVENT_VOLCENTER;
      EventMap[0x22] = EVENT_UNKNOWN3;
      EventMap[0x23] = EVENT_CALLONCE;
      EventMap[0x24] = EVENT_LFOOFF;
      EventMap[0x25] = EVENT_UNKNOWN4;
      EventMap[0x28] = EVENT_PROGCHANGEVOL;
      EventMap[0x29] = EVENT_UNKNOWN1;
      EventMap[0x2a] = EVENT_TIMERFREQ;
      //EventMap[0x2d] = null;
      //EventMap[0x2e] = null;
      EventMap[0x2f] = EVENT_TREMOLO;
      EventMap[0x30] = EVENT_TREMOLOOFF;
      //EventMap[0x31] = EVENT_RESET;
    default:
      break;
  }
}

double RareSnesSeq::GetTempoInBPM(uint8_t _tempo, uint8_t _timerFreq) {
  if (_timerFreq != 0 && _tempo != 0) {
    return 60000000. / (SEQ_PPQN * (125. * _timerFreq)) * (static_cast<double>( _tempo) / 256.);
  }
  else {
    return 1.0; // since tempo 0 cannot be expressed, this function returns a very small value.
  }
}


//  ************
//  RareSnesTrack
//  ************

RareSnesTrack::RareSnesTrack(RareSnesSeq *parentFile, long offset, long length)
    : SeqTrack(parentFile, offset, length) {
  ResetVars();
  bDetermineTrackLengthEventByEvent = true;
  bWriteGenericEventAsTextEvent = false;
}

void RareSnesTrack::ResetVars() {
  SeqTrack::ResetVars();

  cKeyCorrection = SEQ_KEYOFS;

  rptNestLevel = 0;
  spcNotePitch = 0;
  spcTranspose = 0;
  spcTransposeAbs = 0;
  spcInstr = 0;
  spcADSR = 0x8EE0;
  spcTuning = 0;
  defNoteDur = 0;
  useLongDur = false;
  altNoteByte1 = altNoteByte2 = 0x81;
}


double RareSnesTrack::GetTuningInSemitones(int8_t tuning) {
  return 12.0 * log((1024 + tuning) / 1024.0) / log(2.0);
}

void RareSnesTrack::CalcVolPanFromVolLR(int8_t volL, int8_t volR, uint8_t &midiVol, uint8_t &midiPan) {
  double volumeLeft = abs(volL) / 128.0;
  double volumeRight = abs(volR) / 128.0;

  double volumeScale;
  uint8_t midiPanTemp = ConvertVolumeBalanceToStdMidiPan(volumeLeft, volumeRight, &volumeScale);
  volumeScale *= sqrt(3) / 2.0; // limit to <= 1.0

  midiVol = ConvertPercentAmpToStdMidiVal(volumeScale);
  if (volL != 0 || volR != 0) {
    midiPan = midiPanTemp;
  }
}

bool RareSnesTrack::ReadEvent() {
  RareSnesSeq *_parentSeq = dynamic_cast<RareSnesSeq *>(this->parentSeq);
  uint32_t beginOffset = curOffset;
  if (curOffset >= 0x10000) {
    return false;
  }

  uint8_t statusByte = GetByte(curOffset++);
  uint8_t newMidiVol, newMidiPan;
  bool bContinue = true;

  wstringstream desc;

  if (statusByte >= 0x80) {
    uint8_t noteByte = statusByte;

    // check for "reuse last key"
    if (noteByte == 0xe1) {
      noteByte = altNoteByte2;
    }
    else if (noteByte >= 0xe0) {
      noteByte = altNoteByte1;
    }

    uint8_t key = noteByte - 0x81;
    uint8_t spcKey = min(max(noteByte - 0x80 + 36 + spcTranspose, 0), 0x7f);

    uint16_t dur;
    if (defNoteDur != 0) {
      dur = defNoteDur;
    }
    else {
      if (useLongDur) {
        dur = GetShortBE(curOffset);
        curOffset += 2;
      }
      else {
        dur = GetByte(curOffset++);
      }
    }

    if (noteByte == 0x80) {
      //wostringstream ssTrace;
      //ssTrace << L"Rest: " << dur << L" " << defNoteDur << L" " << (useLongDur ? L"L" : L"S") << std::endl;
      //LogDebug(ssTrace.str());

      AddRest(beginOffset, curOffset - beginOffset, dur);
    }
    else {
      // a note, add hints for instrument
      int8_t instrTuningDelta = 0;
      if (_parentSeq->instrUnityKeyHints.find(spcInstr) == _parentSeq->instrUnityKeyHints.end()) {
        _parentSeq->instrUnityKeyHints[spcInstr] = spcTransposeAbs;
        _parentSeq->instrPitchHints[spcInstr] = roundi(GetTuningInSemitones(spcTuning) * 100.0);
      }
      else {
        // check difference between preserved tuning and current tuning
        // example case: Donkey Kong Country 2 - Forest Interlude (Pads)
        instrTuningDelta = spcTransposeAbs - _parentSeq->instrUnityKeyHints[spcInstr];
      }
      if (_parentSeq->instrADSRHints.find(spcInstr) == _parentSeq->instrADSRHints.end()) {
        _parentSeq->instrADSRHints[spcInstr] = spcADSR;
      }

      spcNotePitch = RareSnesSeq::NOTE_PITCH_TABLE[spcKey];
      spcNotePitch = (spcNotePitch * (1024 + spcTuning) + (spcTuning < 0 ? 1023 : 0)) / 1024;

      //wostringstream ssTrace;
      //ssTrace << L"Note: " << key << L" " << dur << L" " << defNoteDur << L" " << (useLongDur ? L"L" : L"S") << L" P=" << spcNotePitch << std::endl;
      //LogDebug(ssTrace.str());

      uint8_t vel = 127;
      AddNoteByDur(beginOffset, curOffset - beginOffset, key + instrTuningDelta, vel, dur);
      AddTime(dur);
    }
  }
  else {
    auto eventType = static_cast<RareSnesSeqEventType>(0);
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

      case EVENT_END:
        AddEndOfTrack(beginOffset, curOffset - beginOffset);
        bContinue = false;
        //loaded = true;
        break;

      case EVENT_PROGCHANGE: {
        uint8_t newProg = GetByte(curOffset++);
        spcInstr = newProg;
        AddProgramChange(beginOffset, curOffset - beginOffset, newProg, true);
        break;
      }

      case EVENT_PROGCHANGEVOL: {
        uint8_t newProg = GetByte(curOffset++);
        int8_t newVolL = GetByte(curOffset++);
        int8_t newVolR = GetByte(curOffset++);

        spcInstr = newProg;
        spcVolL = newVolL;
        spcVolR = newVolR;
        AddProgramChange(beginOffset, curOffset - beginOffset, newProg, true, L"Program Change, Volume");
        AddVolLRNoItem(spcVolL, spcVolR);
        break;
      }

      case EVENT_VOLLR: {
        int8_t newVolL = GetByte(curOffset++);
        int8_t newVolR = GetByte(curOffset++);

        spcVolL = newVolL;
        spcVolR = newVolR;
        AddVolLR(beginOffset, curOffset - beginOffset, spcVolL, spcVolR, L"Volume L/R");
        break;
      }

      case EVENT_VOLCENTER: {
        int8_t newVol = GetByte(curOffset++);

        spcVolL = newVol;
        spcVolR = newVol;
        AddVolLR(beginOffset, curOffset - beginOffset, spcVolL, spcVolR, L"Volume");
        break;
      }

      case EVENT_GOTO: {
        uint16_t dest = GetShort(curOffset);
        curOffset += 2;
        desc << L"Destination: $" << std::hex << std::setfill(L'0') << std::setw(4) << std::uppercase << dest;
        uint32_t length = curOffset - beginOffset;

        curOffset = dest;
        if (!IsOffsetUsed(dest) || rptNestLevel != 0) // nest level check is required for Stickerbrush Symphony
          AddGenericEvent(beginOffset, length, L"Jump", desc.str(), CLR_LOOPFOREVER);
        else
          bContinue = AddLoopForever(beginOffset, length, L"Jump");
        break;
      }

      case EVENT_CALLNTIMES: {
        uint8_t times = GetByte(curOffset++);
        uint16_t dest = GetShort(curOffset);
        curOffset += 2;

        desc << L"Times: " << times << L"  Destination: $" << std::hex << std::setfill(L'0') << std::setw(4)
            << std::uppercase << dest;
        AddGenericEvent(beginOffset,
                        curOffset - beginOffset,
                        (times == 1 ? L"Pattern Play" : L"Pattern Repeat"),
                        desc.str(),
                        CLR_LOOP,
                        ICON_STARTREP);

        if (rptNestLevel == RARESNES_RPTNESTMAX) {
          pRoot->AddLogItem(new LogItem(L"Subroutine nest level overflow\n", LOG_LEVEL_ERR, L"RareSnesSeq"));
          bContinue = false;
          break;
        }

        rptRetnAddr[rptNestLevel] = curOffset;
        rptCount[rptNestLevel] = times;
        rptStart[rptNestLevel] = dest;
        rptNestLevel++;
        curOffset = dest;
        break;
      }

      case EVENT_CALLONCE: {
        uint16_t dest = GetShort(curOffset);
        curOffset += 2;

        desc << L"Destination: $" << std::hex << std::setfill(L'0') << std::setw(4) << std::uppercase << dest;
        AddGenericEvent(beginOffset,
                        curOffset - beginOffset,
                        L"Pattern Play",
                        desc.str(),
                        CLR_LOOP,
                        ICON_STARTREP);

        if (rptNestLevel == RARESNES_RPTNESTMAX) {
          pRoot->AddLogItem(new LogItem(L"Subroutine nest level overflow\n", LOG_LEVEL_ERR, L"RareSnesSeq"));
          bContinue = false;
          break;
        }

        rptRetnAddr[rptNestLevel] = curOffset;
        rptCount[rptNestLevel] = 1;
        rptStart[rptNestLevel] = dest;
        rptNestLevel++;
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

        if (rptNestLevel == 0) {
          pRoot->AddLogItem(new LogItem(L"Subroutine nest level overflow\n", LOG_LEVEL_ERR, L"RareSnesSeq"));
          bContinue = false;
          break;
        }

        rptNestLevel--;
        rptCount[rptNestLevel] = (rptCount[rptNestLevel] - 1) & 0xff;
        if (rptCount[rptNestLevel] != 0) {
          // continue
          curOffset = rptStart[rptNestLevel];
          rptNestLevel++;
        }
        else {
          // return
          curOffset = rptRetnAddr[rptNestLevel];
        }
        break;
      }

      case EVENT_DEFDURON: {
        if (useLongDur) {
          defNoteDur = GetShortBE(curOffset);
          curOffset += 2;
        }
        else {
          defNoteDur = GetByte(curOffset++);
        }

        AddGenericEvent(beginOffset,
                        curOffset - beginOffset,
                        L"Default Duration On",
                        desc.str(),
                        CLR_DURNOTE,
                        ICON_NOTE);
        break;
      }

      case EVENT_DEFDUROFF:
        defNoteDur = 0;
        AddGenericEvent(beginOffset,
                        curOffset - beginOffset,
                        L"Default Duration Off",
                        desc.str(),
                        CLR_DURNOTE,
                        ICON_NOTE);
        break;

      case EVENT_PITCHSLIDEUP: {
        curOffset += 5;
        AddGenericEvent(beginOffset,
                        curOffset - beginOffset,
                        L"Pitch Slide Up",
                        desc.str(),
                        CLR_PITCHBEND,
                        ICON_CONTROL);
        break;
      }

      case EVENT_PITCHSLIDEDOWN: {
        curOffset += 5;
        AddGenericEvent(beginOffset,
                        curOffset - beginOffset,
                        L"Pitch Slide Down",
                        desc.str(),
                        CLR_PITCHBEND,
                        ICON_CONTROL);
        break;
      }

      case EVENT_PITCHSLIDEOFF:
        AddGenericEvent(beginOffset,
                        curOffset - beginOffset,
                        L"Pitch Slide Off",
                        desc.str(),
                        CLR_PITCHBEND,
                        ICON_CONTROL);
        break;

      case EVENT_TEMPO: {
        uint8_t newTempo = GetByte(curOffset++);
        _parentSeq->tempo = newTempo;
        AddTempoBPM(beginOffset,
                    curOffset - beginOffset,
                    _parentSeq->GetTempoInBPM(_parentSeq->tempo, _parentSeq->timerFreq));
        break;
      }

      case EVENT_TEMPOADD: {
        int8_t deltaTempo = GetByte(curOffset++);
        _parentSeq->tempo = (_parentSeq->tempo + deltaTempo) & 0xff;
        AddTempoBPM(beginOffset,
                    curOffset - beginOffset,
                    _parentSeq->GetTempoInBPM(_parentSeq->tempo, _parentSeq->timerFreq),
                    L"Tempo Add");
        break;
      }

      case EVENT_VIBRATOSHORT: {
        curOffset += 3;
        AddGenericEvent(beginOffset,
                        curOffset - beginOffset,
                        L"Vibrato (Short)",
                        desc.str(),
                        CLR_MODULATION,
                        ICON_CONTROL);
        break;
      }

      case EVENT_VIBRATOOFF:
        AddGenericEvent(beginOffset,
                        curOffset - beginOffset,
                        L"Vibrato Off",
                        desc.str(),
                        CLR_MODULATION,
                        ICON_CONTROL);
        break;

      case EVENT_VIBRATO: {
        curOffset += 4;
        AddGenericEvent(beginOffset,
                        curOffset - beginOffset,
                        L"Vibrato",
                        desc.str(),
                        CLR_MODULATION,
                        ICON_CONTROL);
        break;
      }

      case EVENT_TREMOLOOFF:
        AddGenericEvent(beginOffset,
                        curOffset - beginOffset,
                        L"Tremolo Off",
                        desc.str(),
                        CLR_MODULATION,
                        ICON_CONTROL);
        break;

      case EVENT_TREMOLO: {
        curOffset += 4;
        AddGenericEvent(beginOffset,
                        curOffset - beginOffset,
                        L"Tremolo",
                        desc.str(),
                        CLR_MODULATION,
                        ICON_CONTROL);
        break;
      }

      case EVENT_ADSR: {
        uint16_t newADSR = GetShortBE(curOffset);
        curOffset += 2;
        spcADSR = newADSR;

        desc << L"ADSR: " << std::hex << std::setfill(L'0') << std::setw(4) << std::uppercase << newADSR;
        AddGenericEvent(beginOffset, curOffset - beginOffset, L"ADSR", desc.str(), CLR_ADSR, ICON_CONTROL);
        break;
      }

      case EVENT_MASTVOL: {
        // TODO: At least it's not Master Volume in Donkey Kong Country 2
        uint8_t newVol = GetByte(curOffset++);
        desc << L"Volume: " << newVol;
        AddGenericEvent(beginOffset, curOffset - beginOffset, L"Master Volume?", desc.str(), CLR_VOLUME, ICON_CONTROL);
        break;
      }

      case EVENT_MASTVOLLR: {
        int8_t newVolL = GetByte(curOffset++);
        int8_t newVolR = GetByte(curOffset++);
        int8_t newVol = min(abs(newVolL) + abs(newVolR), 255) / 2; // workaround: convert to mono
        AddMasterVol(beginOffset, curOffset - beginOffset, newVol, L"Master Volume L/R");
        break;
      }

      case EVENT_TUNING: {
        int8_t newTuning = GetByte(curOffset++);
        spcTuning = newTuning;
        desc << L"Tuning: " << newTuning << L" (" << (GetTuningInSemitones(newTuning) * 100 + 0.5)
            << L" cents)";
        AddGenericEvent(beginOffset,
                        curOffset - beginOffset,
                        L"Tuning",
                        desc.str(),
                        CLR_PITCHBEND,
                        ICON_CONTROL);
        break;
      }

      case EVENT_TRANSPABS: // should be used for pitch correction of instrument
      {
        int8_t newTransp = GetByte(curOffset++);
        spcTranspose = spcTransposeAbs = newTransp;
        //AddTranspose(beginOffset, curOffset-beginOffset, 0, L"Transpose (Abs)");

        // add event without MIDI event
        desc << L"Transpose: " << newTransp;
        AddGenericEvent(beginOffset, curOffset - beginOffset, L"Transpose", desc.str(), CLR_TRANSPOSE, ICON_CONTROL);

        cKeyCorrection = SEQ_KEYOFS;
        break;
      }

      case EVENT_TRANSPREL: {
        int8_t deltaTransp = GetByte(curOffset++);
        spcTranspose = (spcTranspose + deltaTransp) & 0xff;
        //AddTranspose(beginOffset, curOffset-beginOffset, spcTransposeAbs - spcTranspose, L"Transpose (Rel)");

        // add event without MIDI event
        desc << L"Transpose: " << deltaTransp;
        AddGenericEvent(beginOffset,
                        curOffset - beginOffset,
                        L"Transpose (Relative)",
                        desc.str(),
                        CLR_TRANSPOSE,
                        ICON_CONTROL);

        cKeyCorrection += deltaTransp;
        break;
      }

      case EVENT_ECHOPARAM: {
        uint8_t newFeedback = GetByte(curOffset++);
        int8_t newVolL = GetByte(curOffset++);
        int8_t newVolR = GetByte(curOffset++);
        _parentSeq->midiReverb = min(abs(newVolL) + abs(newVolR), 255) / 2;
        // TODO: update MIDI reverb value for each tracks?

        desc << L"Feedback: " << newFeedback << L"  Volume: " << newVolL << L", " << newVolR;
        AddGenericEvent(beginOffset,
                        curOffset - beginOffset,
                        L"Echo Param",
                        desc.str(),
                        CLR_REVERB,
                        ICON_CONTROL);
        break;
      }

      case EVENT_ECHOON:
        AddReverb(beginOffset, curOffset - beginOffset, _parentSeq->midiReverb, L"Echo On");
        break;

      case EVENT_ECHOOFF:
        AddReverb(beginOffset, curOffset - beginOffset, 0, L"Echo Off");
        break;

      case EVENT_ECHOFIR: {
        uint8_t newFIR[8];
        GetBytes(curOffset, 8, newFIR);
        curOffset += 8;

        desc << L"Filter: ";
        for (int iFIRIndex = 0; iFIRIndex < 8; iFIRIndex++) {
          if (iFIRIndex != 0)
            desc << L" ";
          desc << std::hex << std::setfill(L'0') << std::setw(2) << std::uppercase << newFIR[iFIRIndex];
        }

        AddGenericEvent(beginOffset,
                        curOffset - beginOffset,
                        L"Echo FIR",
                        desc.str(),
                        CLR_REVERB,
                        ICON_CONTROL);
        break;
      }

      case EVENT_NOISECLK: {
        uint8_t newCLK = GetByte(curOffset++);
        desc << L"CLK: " << newCLK;
        AddGenericEvent(beginOffset,
                        curOffset - beginOffset,
                        L"Noise Frequency",
                        desc.str(),
                        CLR_CHANGESTATE,
                        ICON_CONTROL);
        break;
      }

      case EVENT_NOISEON:
        AddGenericEvent(beginOffset,
                        curOffset - beginOffset,
                        L"Noise On",
                        desc.str(),
                        CLR_CHANGESTATE,
                        ICON_CONTROL);
        break;

      case EVENT_NOISEOFF:
        AddGenericEvent(beginOffset,
                        curOffset - beginOffset,
                        L"Noise Off",
                        desc.str(),
                        CLR_CHANGESTATE,
                        ICON_CONTROL);
        break;

      case EVENT_SETALTNOTE1:
        altNoteByte1 = GetByte(curOffset++);
        desc << L"Note: " << std::hex << std::setfill(L'0') << std::setw(2) << std::uppercase << altNoteByte1;
        AddGenericEvent(beginOffset,
                        curOffset - beginOffset,
                        L"Set Alt Note 1",
                        desc.str(),
                        CLR_CHANGESTATE,
                        ICON_NOTE);
        break;

      case EVENT_SETALTNOTE2:
        altNoteByte2 = GetByte(curOffset++);
        desc << L"Note: " << std::hex << std::setfill(L'0') << std::setw(2) << std::uppercase << altNoteByte2;
        AddGenericEvent(beginOffset,
                        curOffset - beginOffset,
                        L"Set Alt Note 2",
                        desc.str(),
                        CLR_CHANGESTATE,
                        ICON_NOTE);
        break;

      case EVENT_PITCHSLIDEDOWNSHORT: {
        curOffset += 4;
        AddGenericEvent(beginOffset,
                        curOffset - beginOffset,
                        L"Pitch Slide Down (Short)",
                        desc.str(),
                        CLR_PITCHBEND,
                        ICON_CONTROL);
        break;
      }

      case EVENT_PITCHSLIDEUPSHORT: {
        curOffset += 4;
        AddGenericEvent(beginOffset,
                        curOffset - beginOffset,
                        L"Pitch Slide Up (Short)",
                        desc.str(),
                        CLR_PITCHBEND,
                        ICON_CONTROL);
        break;
      }

      case EVENT_LONGDURON:
        useLongDur = true;
        AddGenericEvent(beginOffset,
                        curOffset - beginOffset,
                        L"Long Duration On",
                        desc.str(),
                        CLR_DURNOTE,
                        ICON_NOTE);
        break;

      case EVENT_LONGDUROFF:
        useLongDur = false;
        AddGenericEvent(beginOffset,
                        curOffset - beginOffset,
                        L"Long Duration Off",
                        desc.str(),
                        CLR_DURNOTE,
                        ICON_NOTE);
        break;

      case EVENT_SETVOLADSRPRESET1: {
        int8_t newVolL = GetByte(curOffset++);
        int8_t newVolR = GetByte(curOffset++);
        uint8_t adsr1 = GetByte(curOffset++);
        uint8_t adsr2 = GetByte(curOffset++);

        uint8_t ar = adsr1 & 0x0f;
        uint8_t dr = (adsr1 & 0x70) >> 4;
        uint8_t sl = (adsr2 & 0xe0) >> 5;
        uint8_t sr = adsr2 & 0x1f;

        _parentSeq->presetVolL[0] = newVolL;
        _parentSeq->presetVolR[0] = newVolR;
        _parentSeq->presetADSR[0] = (adsr1 << 8) | adsr2;

        // add event without MIDI events
        CalcVolPanFromVolLR(spcVolL, spcVolR, newMidiVol, newMidiPan);
        desc << L"Left Volume: " << newVolL << L"  Right Volume: " << newVolR << L"  AR: " << ar << L"  DR: "
            << dr << L"  SL: " << sl << L"  SR: " << sr;
        AddGenericEvent(beginOffset,
                        curOffset - beginOffset,
                        L"Set Vol/ADSR Preset 1",
                        desc.str(),
                        CLR_VOLUME,
                        ICON_CONTROL);
        break;
      }

      case EVENT_SETVOLADSRPRESET2: {
        int8_t newVolL = GetByte(curOffset++);
        int8_t newVolR = GetByte(curOffset++);
        uint8_t adsr1 = GetByte(curOffset++);
        uint8_t adsr2 = GetByte(curOffset++);

        uint8_t ar = adsr1 & 0x0f;
        uint8_t dr = (adsr1 & 0x70) >> 4;
        uint8_t sl = (adsr2 & 0xe0) >> 5;
        uint8_t sr = adsr2 & 0x1f;

        _parentSeq->presetVolL[1] = newVolL;
        _parentSeq->presetVolR[1] = newVolR;
        _parentSeq->presetADSR[1] = (adsr1 << 8) | adsr2;

        // add event without MIDI events
        CalcVolPanFromVolLR(spcVolL, spcVolR, newMidiVol, newMidiPan);
        desc << L"Left Volume: " << newVolL << L"  Right Volume: " << newVolR << L"  AR: " << ar << L"  DR: "
            << dr << L"  SL: " << sl << L"  SR: " << sr;
        AddGenericEvent(beginOffset,
                        curOffset - beginOffset,
                        L"Set Vol/ADSR Preset 2",
                        desc.str(),
                        CLR_VOLUME,
                        ICON_CONTROL);
        break;
      }

      case EVENT_SETVOLADSRPRESET3: {
        int8_t newVolL = GetByte(curOffset++);
        int8_t newVolR = GetByte(curOffset++);
        uint8_t adsr1 = GetByte(curOffset++);
        uint8_t adsr2 = GetByte(curOffset++);

        uint8_t ar = adsr1 & 0x0f;
        uint8_t dr = (adsr1 & 0x70) >> 4;
        uint8_t sl = (adsr2 & 0xe0) >> 5;
        uint8_t sr = adsr2 & 0x1f;

        _parentSeq->presetVolL[2] = newVolL;
        _parentSeq->presetVolR[2] = newVolR;
        _parentSeq->presetADSR[2] = (adsr1 << 8) | adsr2;

        // add event without MIDI events
        CalcVolPanFromVolLR(spcVolL, spcVolR, newMidiVol, newMidiPan);
        desc << L"Left Volume: " << newVolL << L"  Right Volume: " << newVolR << L"  AR: " << ar << L"  DR: "
            << dr << L"  SL: " << sl << L"  SR: " << sr;
        AddGenericEvent(beginOffset,
                        curOffset - beginOffset,
                        L"Set Vol/ADSR Preset 3",
                        desc.str(),
                        CLR_VOLUME,
                        ICON_CONTROL);
        break;
      }

      case EVENT_SETVOLADSRPRESET4: {
        int8_t newVolL = GetByte(curOffset++);
        int8_t newVolR = GetByte(curOffset++);
        uint8_t adsr1 = GetByte(curOffset++);
        uint8_t adsr2 = GetByte(curOffset++);

        uint8_t ar = adsr1 & 0x0f;
        uint8_t dr = (adsr1 & 0x70) >> 4;
        uint8_t sl = (adsr2 & 0xe0) >> 5;
        uint8_t sr = adsr2 & 0x1f;

        _parentSeq->presetVolL[3] = newVolL;
        _parentSeq->presetVolR[3] = newVolR;
        _parentSeq->presetADSR[3] = (adsr1 << 8) | adsr2;

        // add event without MIDI events
        CalcVolPanFromVolLR(spcVolL, spcVolR, newMidiVol, newMidiPan);
        desc << L"Left Volume: " << newVolL << L"  Right Volume: " << newVolR << L"  AR: " << ar << L"  DR: "
            << dr << L"  SL: " << sl << L"  SR: " << sr;
        AddGenericEvent(beginOffset,
                        curOffset - beginOffset,
                        L"Set Vol/ADSR Preset 4",
                        desc.str(),
                        CLR_VOLUME,
                        ICON_CONTROL);
        break;
      }

      case EVENT_SETVOLADSRPRESET5: {
        int8_t newVolL = GetByte(curOffset++);
        int8_t newVolR = GetByte(curOffset++);
        uint8_t adsr1 = GetByte(curOffset++);
        uint8_t adsr2 = GetByte(curOffset++);

        uint8_t ar = adsr1 & 0x0f;
        uint8_t dr = (adsr1 & 0x70) >> 4;
        uint8_t sl = (adsr2 & 0xe0) >> 5;
        uint8_t sr = adsr2 & 0x1f;

        _parentSeq->presetVolL[4] = newVolL;
        _parentSeq->presetVolR[4] = newVolR;
        _parentSeq->presetADSR[4] = (adsr1 << 8) | adsr2;

        // add event without MIDI events
        CalcVolPanFromVolLR(spcVolL, spcVolR, newMidiVol, newMidiPan);
        desc << L"Left Volume: " << newVolL << L"  Right Volume: " << newVolR << L"  AR: " << ar << L"  DR: "
            << dr << L"  SL: " << sl << L"  SR: " << sr;
        AddGenericEvent(beginOffset,
                        curOffset - beginOffset,
                        L"Set Vol/ADSR Preset 5",
                        desc.str(),
                        CLR_VOLUME,
                        ICON_CONTROL);
        break;
      }

      case EVENT_GETVOLADSRPRESET1:
        spcVolL = _parentSeq->presetVolL[0];
        spcVolR = _parentSeq->presetVolR[0];
        AddVolLR(beginOffset, curOffset - beginOffset, spcVolL, spcVolR, L"Get Vol/ADSR Preset 1");
        break;

      case EVENT_GETVOLADSRPRESET2:
        spcVolL = _parentSeq->presetVolL[1];
        spcVolR = _parentSeq->presetVolR[1];
        AddVolLR(beginOffset, curOffset - beginOffset, spcVolL, spcVolR, L"Get Vol/ADSR Preset 2");
        break;

      case EVENT_GETVOLADSRPRESET3:
        spcVolL = _parentSeq->presetVolL[2];
        spcVolR = _parentSeq->presetVolR[2];
        AddVolLR(beginOffset, curOffset - beginOffset, spcVolL, spcVolR, L"Get Vol/ADSR Preset 3");
        break;

      case EVENT_GETVOLADSRPRESET4:
        spcVolL = _parentSeq->presetVolL[3];
        spcVolR = _parentSeq->presetVolR[3];
        AddVolLR(beginOffset, curOffset - beginOffset, spcVolL, spcVolR, L"Get Vol/ADSR Preset 4");
        break;

      case EVENT_GETVOLADSRPRESET5:
        spcVolL = _parentSeq->presetVolL[4];
        spcVolR = _parentSeq->presetVolR[4];
        AddVolLR(beginOffset, curOffset - beginOffset, spcVolL, spcVolR, L"Get Vol/ADSR Preset 5");
        break;

      case EVENT_TIMERFREQ: {
        uint8_t newFreq = GetByte(curOffset++);
        _parentSeq->timerFreq = newFreq;
        AddTempoBPM(beginOffset,
                    curOffset - beginOffset,
                    _parentSeq->GetTempoInBPM(_parentSeq->tempo, _parentSeq->timerFreq),
                    L"Timer Frequency");
        break;
      }

        //case EVENT_CONDJUMP:
        //	break;

        //case EVENT_SETCONDJUMPPARAM:
        //	break;

      case EVENT_RESETADSR:
        spcADSR = 0x8FE0;
        AddGenericEvent(beginOffset, curOffset - beginOffset, L"Reset ADSR", L"ADSR: 8FE0", CLR_ADSR, ICON_CONTROL);
        break;

      case EVENT_RESETADSRSOFT:
        spcADSR = 0x8EE0;
        AddGenericEvent(beginOffset,
                        curOffset - beginOffset,
                        L"Reset ADSR (Soft)",
                        L"ADSR: 8EE0",
                        CLR_ADSR,
                        ICON_CONTROL);
        break;

      case EVENT_VOICEPARAMSHORT: {
        uint8_t newProg = GetByte(curOffset++);
        int8_t newTransp = GetByte(curOffset++);
        int8_t newTuning = GetByte(curOffset++);

        desc << L"Program Number: " << newProg << L"  Transpose: " << newTransp << L"  Tuning: "
            << newTuning << L" (" << (GetTuningInSemitones(newTuning) * 100 + 0.5) << L" cents)";;

        // instrument
        spcInstr = newProg;
        AddProgramChange(beginOffset, curOffset - beginOffset, newProg, true, L"Program Change, Transpose, Tuning");

        // transpose
        spcTranspose = spcTransposeAbs = newTransp;
        cKeyCorrection = SEQ_KEYOFS;

        // tuning
        spcTuning = newTuning;
        break;
      }

      case EVENT_VOICEPARAM: {
        uint8_t newProg = GetByte(curOffset++);
        int8_t newTransp = GetByte(curOffset++);
        int8_t newTuning = GetByte(curOffset++);
        int8_t newVolL = GetByte(curOffset++);
        int8_t newVolR = GetByte(curOffset++);
        uint16_t newADSR = GetShortBE(curOffset);
        curOffset += 2;

        desc << L"Program Number: " << newProg << L"  Transpose: " << newTransp << L"  Tuning: "
            << newTuning << L" (" << (GetTuningInSemitones(newTuning) * 100 + 0.5) << L" cents)";;
        desc << L"  Volume: " << newVolL << L", " << newVolR;
        desc << L"  ADSR: " << std::hex << std::setfill(L'0') << std::setw(4) << std::uppercase << newADSR;

        // instrument
        spcInstr = newProg;
        AddProgramChange(beginOffset,
                         curOffset - beginOffset,
                         newProg,
                         true,
                         L"Program Change, Transpose, Tuning, Volume L/R, ADSR");

        // transpose
        spcTranspose = spcTransposeAbs = newTransp;
        cKeyCorrection = SEQ_KEYOFS;

        // tuning
        spcTuning = newTuning;

        // volume
        spcVolL = newVolL;
        spcVolR = newVolR;
        AddVolLRNoItem(spcVolL, spcVolR);

        // ADSR
        spcADSR = newADSR;

        break;
      }

      case EVENT_ECHODELAY: {
        uint8_t newEDL = GetByte(curOffset++);
        desc << L"Delay: " << newEDL;
        AddGenericEvent(beginOffset,
                        curOffset - beginOffset,
                        L"Echo Delay",
                        desc.str(),
                        CLR_REVERB,
                        ICON_CONTROL);
        break;
      }

      case EVENT_SETVOLPRESETS: {
        int8_t newVolL1 = GetByte(curOffset++);
        int8_t newVolR1 = GetByte(curOffset++);
        int8_t newVolL2 = GetByte(curOffset++);
        int8_t newVolR2 = GetByte(curOffset++);

        _parentSeq->presetVolL[0] = newVolL1;
        _parentSeq->presetVolR[0] = newVolR1;
        _parentSeq->presetVolL[1] = newVolL2;
        _parentSeq->presetVolR[1] = newVolR2;

        // add event without MIDI events
        CalcVolPanFromVolLR(_parentSeq->presetVolL[0], _parentSeq->presetVolR[0], newMidiVol, newMidiPan);
        desc << L"Left Volume 1: " << newVolL1 << L"  Right Volume 1: " << newVolR1 << L"  Left Volume 2: " << newVolL2
            << L"  Right Volume 2: " << newVolR2;
        AddGenericEvent(beginOffset,
                        curOffset - beginOffset,
                        L"Set Volume Preset",
                        desc.str(),
                        CLR_VOLUME,
                        ICON_CONTROL);
        break;
      }

      case EVENT_GETVOLPRESET1:
        spcVolL = _parentSeq->presetVolL[0];
        spcVolR = _parentSeq->presetVolR[0];
        AddVolLR(beginOffset, curOffset - beginOffset, spcVolL, spcVolR, L"Get Volume Preset 1");
        break;

      case EVENT_GETVOLPRESET2:
        spcVolL = _parentSeq->presetVolL[1];
        spcVolR = _parentSeq->presetVolR[1];
        AddVolLR(beginOffset, curOffset - beginOffset, spcVolL, spcVolR, L"Get Volume Preset 2");
        break;

      case EVENT_LFOOFF:
        AddGenericEvent(beginOffset,
                        curOffset - beginOffset,
                        L"Pitch Slide/Vibrato/Tremolo Off",
                        desc.str(),
                        CLR_CHANGESTATE,
                        ICON_CONTROL);
        break;

      default:
        desc << L"Event: 0x" << std::hex << std::setfill(L'0') << std::setw(2) << std::uppercase << statusByte;
        AddUnknown(beginOffset, curOffset - beginOffset, L"Unknown Event", desc.str());
        pRoot->AddLogItem(new LogItem((std::wstring(L"Unknown Event - ") + desc.str()).c_str(),
                                      LOG_LEVEL_ERR,
                                      L"RareSnesSeq"));
        bContinue = false;
        break;
    }
  }

  //wostringstream ssTrace;
  //ssTrace << L"" << std::hex << std::setfill(L'0') << std::setw(8) << std::uppercase << beginOffset << L": " << std::setw(2) <<statusByte  << L" -> " << std::setw(8) << curOffset << std::endl;
  //LogDebug(ssTrace.str());

  return bContinue;
}

void RareSnesTrack::OnTickBegin() {
}

void RareSnesTrack::OnTickEnd() {
}

void RareSnesTrack::AddVolLR(uint32_t offset,
                             uint32_t length,
                             int8_t _spcVolL,
                             int8_t _spcVolR,
                             const std::wstring &sEventName) {
  uint8_t newMidiVol;
  uint8_t newMidiPan;
  CalcVolPanFromVolLR(_spcVolL, _spcVolR, newMidiVol, newMidiPan);

  std::wostringstream desc;
  desc << L"Left Volume: " << _spcVolL << L"  Right Volume: " << _spcVolR;
  AddGenericEvent(offset, length, sEventName, desc.str(), CLR_VOLUME, ICON_CONTROL);

  // add MIDI events only if updated
  if (newMidiVol != vol) {
    AddVolNoItem(newMidiVol);
  }
  if (newMidiVol != 0 && newMidiPan != prevPan) {
    AddPanNoItem(newMidiPan);
  }
}

void RareSnesTrack::AddVolLRNoItem(int8_t _spcVolL, int8_t _spcVolR) {
  uint8_t newMidiVol;
  uint8_t newMidiPan;
  CalcVolPanFromVolLR(_spcVolL, _spcVolR, newMidiVol, newMidiPan);

  // add MIDI events only if updated
  if (newMidiVol != vol) {
    AddVolNoItem(newMidiVol);
  }
  if (newMidiVol != 0 && newMidiPan != prevPan) {
    AddPanNoItem(newMidiPan);
  }
}
