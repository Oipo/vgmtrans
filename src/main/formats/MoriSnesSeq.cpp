#include "pch.h"
#include "MoriSnesSeq.h"

#include <utility>

DECLARE_FORMAT(MoriSnes);

//  ***********
//  MoriSnesSeq
//  ***********
#define MAX_TRACKS  10
#define SEQ_PPQN    48

MoriSnesSeq::MoriSnesSeq(RawFile *file, MoriSnesVersion ver, uint32_t seqdataOffset, std::wstring newName)
    : VGMSeq(MoriSnesFormat::name, file, seqdataOffset, 0, std::move(newName)),
      version(ver) {
  bLoadTickByTick = true;
  bAllowDiscontinuousTrackData = true;
  bUseLinearAmplitudeScale = true;

  UseReverb();
  AlwaysWriteInitialReverb(0);

  LoadEventMap();
}

MoriSnesSeq::~MoriSnesSeq() = default;

void MoriSnesSeq::ResetVars() {
  VGMSeq::ResetVars();

  spcTempo = 0x20;
  fastTempo = false;

  InstrumentAddresses.clear();
  InstrumentHints.clear();
}

bool MoriSnesSeq::GetHeaderInfo() {
  SetPPQN(SEQ_PPQN);

  uint32_t curOffset = dwOffset;
  VGMHeader *header = AddHeader(dwOffset, 0);

  // reset track start addresses
  for (unsigned short & TrackStartAddres : TrackStartAddress) {
    TrackStartAddres = 0;
  }

  // parse header events
  while (true) {
    uint32_t beginOffset = curOffset;
    uint8_t statusByte = GetByte(curOffset++);
    if (statusByte == 0xff) {
      header->AddSimpleItem(beginOffset, curOffset - beginOffset, L"Header End");
      break;
    }

    if (statusByte <= 0x7f) {
      uint8_t trackIndex = statusByte;
      if (trackIndex > MAX_TRACKS) {
        // out of range
        return false;
      }

      if (curOffset + 2 > 0x10000) {
        return false;
      }

      uint16_t ofsTrackStart = GetShort(curOffset);
      curOffset += 2;
      TrackStartAddress[trackIndex] = curOffset + ofsTrackStart;

      std::wstringstream trackName;
      trackName << L"Track " << (trackIndex + 1) << L" Offset";
      header->AddSimpleItem(beginOffset, curOffset - beginOffset, trackName.str());
    }
    else {
      header->AddUnknownItem(beginOffset, curOffset - beginOffset);
      break;
    }

    if (curOffset + 1 > 0x10000) {
      return false;
    }
  }

  header->SetGuessedLength();
  return true;
}

bool MoriSnesSeq::GetTrackPointers() {
  for (unsigned short TrackStartAddres : TrackStartAddress) {
    if (TrackStartAddres != 0) {
      MoriSnesTrack *track = new MoriSnesTrack(this, TrackStartAddres);
      aTracks.push_back(track);
    }
  }
  return true;
}

void MoriSnesSeq::LoadEventMap() {
  int statusByte;

  for (statusByte = 0x00; statusByte <= 0x7f; statusByte++) {
    EventMap[statusByte] = EVENT_NOTE_PARAM;
  }

  for (statusByte = 0x80; statusByte <= 0x9f; statusByte++) {
    EventMap[statusByte] = EVENT_NOTE;
  }

  for (statusByte = 0xa0; statusByte <= 0xbf; statusByte++) {
    EventMap[statusByte] = EVENT_NOTE_WITH_PARAM;
  }

  EventMap[0xc0] = EVENT_PROGCHANGE;
  EventMap[0xc1] = EVENT_PAN;
  EventMap[0xc2] = EVENT_UNKNOWN1;
  EventMap[0xc3] = EVENT_TEMPO;
  EventMap[0xc4] = EVENT_UNKNOWN1;
  EventMap[0xc5] = EVENT_VOLUME;
  EventMap[0xc6] = EVENT_PRIORITY;
  EventMap[0xc7] = EVENT_TUNING;
  EventMap[0xc8] = EVENT_ECHO_ON;
  EventMap[0xc9] = EVENT_ECHO_OFF;
  EventMap[0xca] = EVENT_ECHO_PARAM;
  EventMap[0xcb] = EVENT_GOTO;
  EventMap[0xcc] = EVENT_CALL;
  EventMap[0xcd] = EVENT_RET;
  EventMap[0xce] = EVENT_LOOP_START;
  EventMap[0xcf] = EVENT_LOOP_END;
  EventMap[0xd0] = EVENT_END;
  EventMap[0xd1] = EVENT_NOTE_NUMBER;
  EventMap[0xd2] = EVENT_OCTAVE_UP;
  EventMap[0xd3] = EVENT_OCTAVE_DOWN;
  EventMap[0xd4] = EVENT_WAIT;
  EventMap[0xd5] = EVENT_UNKNOWN1;
  EventMap[0xd6] = EVENT_PITCHBENDRANGE;
  EventMap[0xd7] = EVENT_TRANSPOSE;
  EventMap[0xd8] = EVENT_TRANSPOSE_REL;
  EventMap[0xd9] = EVENT_TUNING_REL;
  EventMap[0xda] = EVENT_KEY_ON;
  EventMap[0xdb] = EVENT_KEY_OFF;
  EventMap[0xdc] = EVENT_VOLUME_REL;
  EventMap[0xdd] = EVENT_PITCHBEND;
  EventMap[0xde] = EVENT_INSTR;
  EventMap[0xdf] = EVENT_UNKNOWN1;
  EventMap[0xe0] = EVENT_UNKNOWN1;
  EventMap[0xe1] = EVENT_UNKNOWN1;
  EventMap[0xe2] = EVENT_UNKNOWN1;
  EventMap[0xe3] = EVENT_UNKNOWN1;
  EventMap[0xe4] = EVENT_UNKNOWN1;
  EventMap[0xe5] = EVENT_UNKNOWN1;
  EventMap[0xe6] = EVENT_TIMEBASE;
}

double MoriSnesSeq::GetTempoInBPM(uint8_t tempo, bool _fastTempo) {
  if (tempo != 0) {
    return 60000000.0 / ((SEQ_PPQN / (_fastTempo ? 2 : 1)) * (125 * 0x4f)) * (tempo / 256.0);
  }
  else {
    return 1.0; // since tempo 0 cannot be expressed, this function returns a very small value.
  }
}

//  ***************
//  MoriSnesTrack
//  ***************

MoriSnesTrack::MoriSnesTrack(MoriSnesSeq *parentFile, long offset, long length)
    : SeqTrack(parentFile, offset, length) {
  ResetVars();
  bDetermineTrackLengthEventByEvent = true;
  bWriteGenericEventAsTextEvent = false;
}

void MoriSnesTrack::ResetVars() {
  SeqTrack::ResetVars();

  tiedNoteKeys.clear();
  spcDeltaTime = 0;
  spcNoteNumberBase = 0;
  spcNoteDuration = 1;
  spcNoteVelocity = 1;
  spcVolume = 200;
  spcTranspose = 0;
  spcTuning = 0;
  spcCallStackPtr = 0;
}


bool MoriSnesTrack::ReadEvent() {
  MoriSnesSeq *_parentSeq = dynamic_cast<MoriSnesSeq *>(this->parentSeq);

  uint32_t beginOffset = curOffset;
  if (curOffset >= 0x10000) {
    return false;
  }

  uint8_t statusByte = GetByte(curOffset++);
  bool bContinue = true;

  std::wstringstream desc;

  // note param
  uint8_t newDelta = spcDeltaTime;
  if (statusByte < 0x80) {
    newDelta = statusByte;
    desc << L"Delta Time: " << newDelta;
    if (newDelta != 0) {
      spcDeltaTime = newDelta;
    }

    statusByte = GetByte(curOffset);
    if (statusByte < 0x80) {
      spcNoteDuration = statusByte;
      desc << L"  Dulation: " << spcNoteDuration;
      curOffset++;

      statusByte = GetByte(curOffset);
      if (statusByte < 0x80) {
        spcNoteVelocity = statusByte;
        desc << L"  Velocity: " << spcNoteVelocity;
        curOffset++;
      }
    }

    AddGenericEvent(beginOffset, curOffset - beginOffset, L"Note Param", desc.str(), CLR_DURNOTE);
    beginOffset = curOffset;
    desc.str(L"");

    if (curOffset >= 0x10000) {
      return false;
    }

    statusByte = GetByte(curOffset++);
  }

  auto eventType = static_cast<MoriSnesSeqEventType>(0);
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

    case EVENT_NOTE:
    case EVENT_NOTE_WITH_PARAM: {
      uint8_t keyOffset = statusByte & 0x1f;

      const wchar_t *eventName;
      if (eventType == EVENT_NOTE_WITH_PARAM) {
        uint8_t noteParam = GetByte(curOffset++);
        if (noteParam <= 0x7f) {
          spcNoteDuration = noteParam;
          eventName = L"Note with Duration";
        }
        else {
          spcNoteVelocity = (noteParam & 0x7f);
          eventName = L"Note with Velocity";
        }
      }
      else {
        eventName = L"Note";
      }

      bool tied = (spcNoteDuration == 0);
      uint8_t dur = spcNoteDuration;
      if (tied) {
        dur = spcDeltaTime;
      }

      int8_t key = spcNoteNumberBase + keyOffset;
      auto prevTiedNoteIter = std::find(tiedNoteKeys.begin(), tiedNoteKeys.end(), key);
      if (prevTiedNoteIter != tiedNoteKeys.end()) {
        MakePrevDurNoteEnd(GetTime() + dur);
        AddGenericEvent(beginOffset, curOffset - beginOffset, L"Tie", desc.str(), CLR_TIE, ICON_NOTE);

        if (!tied) {
          // finish tied note
          tiedNoteKeys.erase(prevTiedNoteIter);
        }
      }
      else {
        AddNoteByDur(beginOffset, curOffset - beginOffset, key, spcNoteVelocity, dur, eventName);
        if (tied) {
          // register tied note
          tiedNoteKeys.push_back(key);
        }
      }
      AddTime(newDelta);
      break;
    }

    case EVENT_PROGCHANGE: {
      int16_t instrOffset = GetShort(curOffset);
      curOffset += 2;
      uint16_t instrAddress = curOffset + instrOffset;
      desc << L"Envelope: $" << std::hex << std::setfill(L'0') << std::setw(4) << std::uppercase << instrAddress;

      uint8_t instrNum;
      for (instrNum = 0; instrNum < _parentSeq->InstrumentAddresses.size(); instrNum++) {
        if (_parentSeq->InstrumentAddresses[instrNum] == instrAddress) {
          break;
        }
      }

      // new instrument?
      if (instrNum == _parentSeq->InstrumentAddresses.size()) {
        _parentSeq->InstrumentAddresses.push_back(instrAddress);
        ParseInstrument(instrAddress, instrNum);
      }

      AddGenericEvent(beginOffset,
                      curOffset - beginOffset,
                      L"Program Change",
                      desc.str(),
                      CLR_PROGCHANGE,
                      ICON_PROGCHANGE);
      AddProgramChangeNoItem(instrNum, false);
      break;
    }

    case EVENT_PAN: {
      int8_t newPan = GetByte(curOffset++);
      if (newPan >= 0) {
        if (newPan > 32) {
          // out of range (unexpected behavior)
          newPan = 32;
        }

        uint8_t midiPan = min(newPan * 4, 127);
        AddPan(beginOffset, curOffset - beginOffset, midiPan);
      }
      else {
        AddGenericEvent(beginOffset, curOffset - beginOffset, L"Random Pan", desc.str(), CLR_PAN, ICON_CONTROL);
      }
      break;
    }

    case EVENT_TEMPO: {
      uint8_t newTempo = GetByte(curOffset++);
      _parentSeq->spcTempo = newTempo;
      AddTempoBPM(beginOffset, curOffset - beginOffset,
                  _parentSeq->GetTempoInBPM(newTempo, _parentSeq->fastTempo));
      break;
    }

    case EVENT_VOLUME: {
      uint8_t newVolume = GetByte(curOffset++);
      spcVolume = newVolume;
      AddVol(beginOffset, curOffset - beginOffset, spcVolume / 2);
      break;
    }

    case EVENT_PRIORITY: {
      uint8_t newPriority = GetByte(curOffset++);
      desc << L"Priority: " << newPriority;
      AddGenericEvent(beginOffset, curOffset - beginOffset, L"Priority", desc.str(), CLR_PRIORITY);
      break;
    }

    case EVENT_TUNING: {
      uint8_t newTuning = GetByte(curOffset++);
      spcTuning = newTuning;

      double semitones = spcTuning / 256.0;
      AddFineTuning(beginOffset, curOffset - beginOffset, semitones * 100.0);
      break;
    }

    case EVENT_ECHO_ON: {
      AddGenericEvent(beginOffset, curOffset - beginOffset, L"Echo On", desc.str(), CLR_REVERB, ICON_CONTROL);
      AddReverbNoItem(40);
      break;
    }

    case EVENT_ECHO_OFF: {
      AddGenericEvent(beginOffset, curOffset - beginOffset, L"Echo Off", desc.str(), CLR_REVERB, ICON_CONTROL);
      AddReverbNoItem(0);
      break;
    }

    case EVENT_ECHO_PARAM: {
      uint8_t delay = GetByte(curOffset++);
      int8_t volume = GetByte(curOffset++);
      int8_t feedback = GetByte(curOffset++);
      uint8_t filterIndex = GetByte(curOffset++);
      desc << L"Delay: " << delay << L"  FIR: " << volume << L"  volume: " << feedback << L"  FIR: "
          << filterIndex;
      AddGenericEvent(beginOffset,
                      curOffset - beginOffset,
                      L"Echo Param",
                      desc.str(),
                      CLR_REVERB,
                      ICON_CONTROL);
      break;
    }

    case EVENT_GOTO: {
      uint16_t dest = GetShort(curOffset);
      curOffset += 2;
      dest += curOffset; // relative offset to address
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

    case EVENT_CALL: {
      uint16_t dest = GetShort(curOffset);
      curOffset += 2;
      dest += curOffset; // relative offset to address
      desc << L"Destination: $" << std::hex << std::setfill(L'0') << std::setw(4) << std::uppercase << dest;
      AddGenericEvent(beginOffset,
                      curOffset - beginOffset,
                      L"Pattern Play",
                      desc.str(),
                      CLR_LOOP,
                      ICON_STARTREP);

      if (spcCallStackPtr + 2 > MORISNES_CALLSTACK_SIZE) {
        // stack overflow
        bContinue = false;
        break;
      }

      // save return address
      spcCallStack[spcCallStackPtr++] = curOffset & 0xff;
      spcCallStack[spcCallStackPtr++] = (curOffset >> 8) & 0xff;

      curOffset = dest;

      // TODO: update track address if necessary
      // example: Lennus 2 - Staff Roll $3d47

      break;
    }

    case EVENT_RET: {
      AddGenericEvent(beginOffset, curOffset - beginOffset, L"End Pattern", desc.str(), CLR_LOOP, ICON_ENDREP);

      if (spcCallStackPtr < 2) {
        // access violation
        bContinue = false;
        break;
      }

      curOffset = spcCallStack[spcCallStackPtr - 2] | (spcCallStack[spcCallStackPtr - 1] << 8);
      spcCallStackPtr -= 2;
      break;
    }

    case EVENT_LOOP_START: {
      uint8_t count = GetByte(curOffset++);
      desc << L"Loop Count: " << count;
      AddGenericEvent(beginOffset, curOffset - beginOffset, L"Loop Start", desc.str(), CLR_LOOP, ICON_STARTREP);

      if (spcCallStackPtr + 3 > MORISNES_CALLSTACK_SIZE) {
        // stack overflow
        bContinue = false;
        break;
      }

      // save loop start address and repeat count
      spcCallStack[spcCallStackPtr++] = curOffset & 0xff;
      spcCallStack[spcCallStackPtr++] = (curOffset >> 8) & 0xff;
      spcCallStack[spcCallStackPtr++] = count;

      break;
    }

    case EVENT_LOOP_END: {
      AddGenericEvent(beginOffset, curOffset - beginOffset, L"Loop End", desc.str(), CLR_LOOP, ICON_ENDREP);

      if (spcCallStackPtr < 3) {
        // access violation
        bContinue = false;
        break;
      }

      uint8_t count = spcCallStack[spcCallStackPtr - 1];
      if (--count == 0) {
        // repeat end, fall through
        spcCallStackPtr -= 3;
      }
      else {
        // repeat again
        spcCallStack[spcCallStackPtr - 1] = count;
        curOffset = spcCallStack[spcCallStackPtr - 3] | (spcCallStack[spcCallStackPtr - 2] << 8);
      }

      break;
    }

    case EVENT_END: {
      AddEndOfTrack(beginOffset, curOffset - beginOffset);
      bContinue = false;
      break;
    }

    case EVENT_NOTE_NUMBER: {
      int8_t newNoteNumber = GetByte(curOffset++);
      spcNoteNumberBase = newNoteNumber;
      AddGenericEvent(beginOffset, curOffset - beginOffset, L"Note Number Base", desc.str(), CLR_NOTEON);
      break;
    }

    case EVENT_OCTAVE_UP: {
      AddIncrementOctave(beginOffset, curOffset - beginOffset);
      spcNoteNumberBase += 12;
      break;
    }

    case EVENT_OCTAVE_DOWN: {
      AddDecrementOctave(beginOffset, curOffset - beginOffset);
      spcNoteNumberBase -= 12;
      break;
    }

    case EVENT_WAIT: {
      // do not stop tied note here
      // example: Gokinjo Bouken Tai - Battle (28:0000, Sax at 3rd channel)
      desc << L"Duration: " << spcDeltaTime;
      AddGenericEvent(beginOffset, curOffset - beginOffset, L"Wait", desc.str(), CLR_REST, ICON_REST);
      AddTime(spcDeltaTime);
      break;
    }

    case EVENT_PITCHBENDRANGE: {
      uint8_t newRange = GetByte(curOffset++); // actual range is value/8
      AddPitchBendRange(beginOffset,
                        curOffset - beginOffset,
                        newRange / 8); // range <= 24 is recommended in General MIDI spec
      break;
    }

    case EVENT_TRANSPOSE: {
      int8_t newTranspose = GetByte(curOffset++);
      spcTranspose = newTranspose;
      AddTranspose(beginOffset, curOffset - beginOffset, spcTranspose);
      break;
    }

    case EVENT_TRANSPOSE_REL: {
      int8_t delta = GetByte(curOffset++);
      spcTranspose += delta;
      AddTranspose(beginOffset, curOffset - beginOffset, spcTranspose, L"Transpose (Relative)");
      break;
    }

    case EVENT_TUNING_REL: {
      int8_t delta = GetByte(curOffset++);
      spcTuning += delta;

      double semitones = spcTuning / 256.0;
      AddFineTuning(beginOffset, curOffset - beginOffset, semitones * 100.0);
      break;
    }

    case EVENT_KEY_ON: {
      AddGenericEvent(beginOffset, curOffset - beginOffset, L"Key On", desc.str(), CLR_NOTEON, ICON_NOTE);
      break;
    }

    case EVENT_KEY_OFF: {
      AddGenericEvent(beginOffset, curOffset - beginOffset, L"Key Off", desc.str(), CLR_NOTEOFF, ICON_NOTE);
      break;
    }

    case EVENT_VOLUME_REL: {
      int8_t delta = GetByte(curOffset++);

      int newVolume = min(max(spcVolume + delta, 0), 0xff);
      spcVolume += newVolume;

      AddVol(beginOffset, curOffset - beginOffset, spcVolume / 2, L"Volume (Relative)");
      break;
    }

    case EVENT_PITCHBEND: {
      int8_t pitch = GetByte(curOffset++);
      AddPitchBend(beginOffset, curOffset - beginOffset, pitch * 64);
      break;
    }

    case EVENT_TIMEBASE: {
      bool fast = ((GetByte(curOffset++) & 1) != 0);
      desc << L"Timebase: " << (fast ? SEQ_PPQN : SEQ_PPQN / 2);
      AddGenericEvent(beginOffset, curOffset - beginOffset, L"Timebase", desc.str(), CLR_TEMPO, ICON_TEMPO);

      if (_parentSeq->fastTempo != fast) {
        AddTempoBPMNoItem(_parentSeq->GetTempoInBPM(_parentSeq->spcTempo, fast));
        _parentSeq->fastTempo = fast;
      }
      break;
    }

    default:
      desc << L"Event: 0x" << std::hex << std::setfill(L'0') << std::setw(2) << std::uppercase << statusByte;
      AddUnknown(beginOffset, curOffset - beginOffset, L"Unknown Event", desc.str());
      pRoot->AddLogItem(new LogItem((std::wstring(L"Unknown Event - ") + desc.str()).c_str(),
                                    LOG_LEVEL_ERR,
                                    L"MoriSnesSeq"));
      bContinue = false;
      break;
  }

  //assert(curOffset >= dwOffset);

  //wostringstream ssTrace;
  //ssTrace << L"" << std::hex << std::setfill(L'0') << std::setw(8) << std::uppercase << beginOffset << L": " << std::setw(2) <<statusByte  << L" -> " << std::setw(8) << curOffset << std::endl;
  //OutputDebugString(ssTrace.str());

  // increase delta-time
  //if (eventType == EVENT_NOTE || eventType == EVENT_NOTE_WITH_PARAM) { // rest is
  //	AddTime(spcDeltaTime);
  //}

  return bContinue;
}

void MoriSnesTrack::ParseInstrument(uint16_t instrAddress, uint8_t instrNum) {
  MoriSnesSeq *_parentSeq = dynamic_cast<MoriSnesSeq *>(this->parentSeq);

  uint16_t _curOffset = instrAddress;

  bool percussion = ((GetByte(_curOffset++) & 1) != 0);
  _parentSeq->InstrumentHints[instrAddress].percussion = percussion;

  if (!percussion) {
    ParseInstrumentEvents(_curOffset, instrNum);
  }
  else {
    int16_t instrOffset;
    uint16_t instrPtrAddressMax = 0xffff;
    uint8_t percNoteKey = 0;
    while (_curOffset < instrPtrAddressMax) {
      instrOffset = GetShort(_curOffset);
      _curOffset += 2;
      uint16_t percInstrAddress = _curOffset + instrOffset;
      if (percInstrAddress < instrPtrAddressMax) {
        instrPtrAddressMax = percInstrAddress;
      }

      ParseInstrumentEvents(percInstrAddress, instrNum, percussion, percNoteKey);
      percNoteKey++;
    }
  }
}

void MoriSnesTrack::ParseInstrumentEvents(uint16_t offset, uint8_t instrNum, bool percussion, uint8_t percNoteKey) {
  auto *_parentSeq = dynamic_cast<MoriSnesSeq *>(this->parentSeq);
  uint16_t instrAddress = _parentSeq->InstrumentAddresses[instrNum];

  MoriSnesInstrHint *instrHint;
  if (!percussion) {
    instrHint = &_parentSeq->InstrumentHints[instrAddress].instrHint;
  }
  else {
    _parentSeq->InstrumentHints[instrAddress].percHints.resize(percNoteKey + 1);
    instrHint = &_parentSeq->InstrumentHints[instrAddress].percHints[percNoteKey];
  }

  bool bContinue = true;
  uint16_t _curOffset = offset;
  uint16_t seqStartAddress = _curOffset;
  uint16_t seqEndAddress = _curOffset;

  uint8_t instrDeltaTime = 0;
  uint8_t instrCallStackPtr = 0;
  uint8_t instrCallStack[MORISNES_CALLSTACK_SIZE];

  while (bContinue) {
//    uint16_t beginOffset = _curOffset;
//    if (curOffset >= 0x10000) { // always false because 0x10000 is outside of uint16_t range
//      break;
//    }

    if (_curOffset < seqStartAddress) {
      seqStartAddress = _curOffset;
    }

    uint8_t statusByte = GetByte(_curOffset++);

    uint8_t newDelta = instrDeltaTime;
    if (statusByte < 0x80) {
      newDelta = statusByte;
      if (newDelta != 0) {
        instrDeltaTime = newDelta;
      }

      statusByte = GetByte(_curOffset);
      if (statusByte < 0x80) {
        // duration
        _curOffset++;

        statusByte = GetByte(_curOffset);
        if (statusByte < 0x80) {
          // velocity
          _curOffset++;
        }
      }

//      beginOffset = _curOffset;
//    if (curOffset >= 0x10000) { // always false because 0x10000 is outside of uint16_t range
//        break;
//      }

      statusByte = GetByte(_curOffset++);

      // workaround: handle statusByte < 0x80 in some sequences (64 64 64 64 ...)
      // example: Lennus 2 - Title ($74f5)
      // example: Shin SD Sengokuden: Daishougun R - Dark Army Corps ($c9be)
      if (statusByte < 0x80) {
        statusByte = 0x80 | (statusByte & 0x1f);
      }
    }

    MoriSnesSeqEventType eventType = static_cast<MoriSnesSeqEventType>(0);
    auto pEventType = _parentSeq->EventMap.find(statusByte);
    if (pEventType != _parentSeq->EventMap.end()) {
      eventType = pEventType->second;
    }

    switch (eventType) {
      case EVENT_UNKNOWN0: {
        break;
      }

      case EVENT_UNKNOWN1: {
        _curOffset++;
        break;
      }

      case EVENT_UNKNOWN2: {
        _curOffset += 2;
        break;
      }

      case EVENT_UNKNOWN3: {
        _curOffset += 3;
        break;
      }

      case EVENT_UNKNOWN4: {
        _curOffset += 4;
        break;
      }

      case EVENT_NOTE:
      case EVENT_NOTE_WITH_PARAM: {
        if (eventType == EVENT_NOTE_WITH_PARAM) {
          _curOffset++;
        }
        break;
      }

      case EVENT_PAN: {
        int8_t newPan = GetByte(_curOffset++);
        if (newPan > 32) {
          newPan = 32;
        }
        instrHint->pan = newPan;
        break;
      }

      case EVENT_VOLUME:
      case EVENT_TUNING:
        _curOffset++;
        break;

      case EVENT_GOTO: {
        uint16_t dest = GetShort(_curOffset);
        _curOffset += 2;
        dest += _curOffset; // relative offset to address

        if (dest > _curOffset) {
          // Gokinjo Bouken Tai - Town ($1581)
          _curOffset = dest;
        }
        else {
          // prevent infinite loop
          bContinue = false;
        }

        break;
      }

      case EVENT_CALL: {
        uint16_t dest = GetShort(_curOffset);
        _curOffset += 2;
        dest += _curOffset; // relative offset to address

        if (instrCallStackPtr + 2 > MORISNES_CALLSTACK_SIZE) {
          // stack overflow
          bContinue = false;
          break;
        }

        // save return address
        instrCallStack[instrCallStackPtr++] = _curOffset & 0xff;
        instrCallStack[instrCallStackPtr++] = (_curOffset >> 8) & 0xff;

        _curOffset = dest;
        break;
      }

      case EVENT_RET: {
        if (instrCallStackPtr < 2) {
          // access violation
          bContinue = false;
          break;
        }

        _curOffset = instrCallStack[instrCallStackPtr - 2] | (instrCallStack[instrCallStackPtr - 1] << 8);
        instrCallStackPtr -= 2;
        break;
      }

      case EVENT_LOOP_START: {
        uint8_t count = GetByte(_curOffset++);

        if (instrCallStackPtr + 3 > MORISNES_CALLSTACK_SIZE) {
          // stack overflow
          bContinue = false;
          break;
        }

        // save loop start address and repeat count
        instrCallStack[instrCallStackPtr++] = _curOffset & 0xff;
        instrCallStack[instrCallStackPtr++] = (_curOffset >> 8) & 0xff;
        instrCallStack[instrCallStackPtr++] = count;

        break;
      }

      case EVENT_LOOP_END: {
        if (instrCallStackPtr < 3) {
          // access violation
          bContinue = false;
          break;
        }

        uint8_t count = instrCallStack[instrCallStackPtr - 1];
        if (--count == 0) {
          // repeat end, fall through
          instrCallStackPtr -= 3;
        }
        else {
          // repeat again
          instrCallStack[instrCallStackPtr - 1] = count;
          _curOffset = instrCallStack[instrCallStackPtr - 3] | (instrCallStack[instrCallStackPtr - 2] << 8);
        }

        break;
      }

      case EVENT_END:
        bContinue = false;
        break;

      case EVENT_TRANSPOSE:
        instrHint->transpose = GetByte(_curOffset++);
        break;

      case EVENT_TRANSPOSE_REL: {
        int8_t delta = GetByte(_curOffset++);
        instrHint->transpose += delta;
        break;
      }

      case EVENT_TUNING_REL:
        _curOffset++;
        break;

      case EVENT_KEY_ON:
      case EVENT_KEY_OFF:
        break;

      case EVENT_VOLUME_REL:
        _curOffset++;
        break;

      case EVENT_INSTR: {
        int16_t rgnOffset = GetShort(_curOffset);
        _curOffset += 2;
        uint16_t rgnAddress = _curOffset + rgnOffset;
        instrHint->rgnAddress = rgnAddress;
        break;
      }

      default:
//#ifdef _WIN32
//			std::wostringstream ssTrace;
//			ssTrace << L"" << std::hex << std::setfill(L'0') << std::setw(8) << std::uppercase << beginOffset << L": " << std::setw(2) <<statusByte  << L" -> " << std::setw(8) << curOffset << std::endl;
//			OutputDebugString(ssTrace.str());
//#endif

        bContinue = false;
        break;
    }

    if (_curOffset > seqEndAddress) {
      seqEndAddress = _curOffset;
    }
  }

  instrHint->seqAddress = seqStartAddress;
  instrHint->seqSize = seqEndAddress - seqStartAddress;

  instrHint->startAddress = instrHint->seqAddress;
  instrHint->size = instrHint->seqSize;

  if (instrHint->rgnAddress != 0) {
    if (instrHint->rgnAddress < instrHint->startAddress) {
      instrHint->startAddress = instrHint->rgnAddress;
    }
    if (instrHint->rgnAddress + 7 > instrHint->startAddress + instrHint->size) {
      instrHint->size = (instrHint->rgnAddress + 7) - instrHint->startAddress;
    }
  }
}
