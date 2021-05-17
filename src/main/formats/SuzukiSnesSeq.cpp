#include "pch.h"
#include "SuzukiSnesSeq.h"

#include <utility>

DECLARE_FORMAT(SuzukiSnes);

//  *************
//  SuzukiSnesSeq
//  *************
#define MAX_TRACKS  8
#define SEQ_PPQN    48

const uint8_t SuzukiSnesSeq::NOTE_DUR_TABLE[13] = {
    0xc0, 0x90, 0x60, 0x48, 0x30, 0x24, 0x20, 0x18,
    0x10, 0x0c, 0x08, 0x06, 0x03
};

SuzukiSnesSeq::SuzukiSnesSeq(RawFile *file, SuzukiSnesVersion ver, uint32_t seqdataOffset, std::wstring newName)
    : VGMSeq(SuzukiSnesFormat::name, file, seqdataOffset, 0, std::move(newName)),
      version(ver) {
  bLoadTickByTick = true;
  bAllowDiscontinuousTrackData = true;
  bUseLinearAmplitudeScale = true;

  UseReverb();
  AlwaysWriteInitialReverb(0);

  LoadEventMap();
}

SuzukiSnesSeq::~SuzukiSnesSeq() = default;

void SuzukiSnesSeq::ResetVars() {
  VGMSeq::ResetVars();

  spcTempo = 0x81; // just in case
}

bool SuzukiSnesSeq::GetHeaderInfo() {
  SetPPQN(SEQ_PPQN);

  VGMHeader *header = AddHeader(dwOffset, 0);
  uint32_t curOffset = dwOffset;

  // skip unknown stream
  if (version != SUZUKISNES_SD3) {
    while (true) {
      if (curOffset + 1 >= 0x10000) {
        return false;
      }

      uint8_t firstByte = GetByte(curOffset);
      if (firstByte >= 0x80) {
        header->AddSimpleItem(curOffset, 1, L"Unknown Items End");
        curOffset++;
        break;
      }
      else {
        header->AddUnknownItem(curOffset, 5);
        curOffset += 5;
      }
    }
  }

  // create tracks
  for (int trackIndex = 0; trackIndex < MAX_TRACKS; trackIndex++) {
    uint16_t addrTrackStart = GetShort(curOffset);

    if (addrTrackStart != 0) {
      std::wstringstream trackName;
      trackName << L"Track Pointer " << (trackIndex + 1);
      header->AddSimpleItem(curOffset, 2, trackName.str());

      aTracks.push_back(new SuzukiSnesTrack(this, addrTrackStart));
    }
    else {
      // example: Super Mario RPG - Where Am I Going?
      header->AddSimpleItem(curOffset, 2, L"nullptr");
    }

    curOffset += 2;
  }

  header->SetGuessedLength();

  return true;        //successful
}

bool SuzukiSnesSeq::GetTrackPointers() {
  return true;
}

void SuzukiSnesSeq::LoadEventMap() {
  for (unsigned int statusByte = 0x00; statusByte <= 0xc3; statusByte++) {
    EventMap[statusByte] = EVENT_NOTE;
  }

  EventMap[0xc4] = EVENT_OCTAVE_UP;
  EventMap[0xc5] = EVENT_OCTAVE_DOWN;
  EventMap[0xc6] = EVENT_OCTAVE;
  EventMap[0xc7] = EVENT_NOP;
  EventMap[0xc8] = EVENT_NOISE_FREQ;
  EventMap[0xc9] = EVENT_NOISE_ON;
  EventMap[0xca] = EVENT_NOISE_OFF;
  EventMap[0xcb] = EVENT_PITCH_MOD_ON;
  EventMap[0xcc] = EVENT_PITCH_MOD_OFF;
  EventMap[0xcd] = EVENT_JUMP_TO_SFX_LO;
  EventMap[0xce] = EVENT_JUMP_TO_SFX_HI;
  EventMap[0xcf] = EVENT_TUNING;
  EventMap[0xd0] = EVENT_END;
  EventMap[0xd1] = EVENT_TEMPO;
  if (version == SUZUKISNES_SD3) {
    EventMap[0xd2] = EVENT_LOOP_START; // duplicated
    EventMap[0xd3] = EVENT_LOOP_START; // duplicated
  }
  else {
    EventMap[0xd2] = EVENT_TIMER1_FREQ;
    EventMap[0xd3] = EVENT_TIMER1_FREQ_REL;
  }
  EventMap[0xd4] = EVENT_LOOP_START;
  EventMap[0xd5] = EVENT_LOOP_END;
  EventMap[0xd6] = EVENT_LOOP_BREAK;
  EventMap[0xd7] = EVENT_LOOP_POINT;
  EventMap[0xd8] = EVENT_ADSR_DEFAULT;
  EventMap[0xd9] = EVENT_ADSR_AR;
  EventMap[0xda] = EVENT_ADSR_DR;
  EventMap[0xdb] = EVENT_ADSR_SL;
  EventMap[0xdc] = EVENT_ADSR_SR;
  EventMap[0xdd] = EVENT_DURATION_RATE;
  EventMap[0xde] = EVENT_PROGCHANGE;
  EventMap[0xdf] = EVENT_NOISE_FREQ_REL;
  if (version == SUZUKISNES_SD3) {
    EventMap[0xe0] = EVENT_VOLUME;
  }
  else { // SUZUKISNES_BL, SUZUKISNES_SMR
    EventMap[0xe0] = EVENT_UNKNOWN1;
  }
  //EventMap[0xe1] = (SuzukiSnesSeqEventType)0;
  EventMap[0xe2] = EVENT_VOLUME;
  EventMap[0xe3] = EVENT_VOLUME_REL;
  EventMap[0xe4] = EVENT_VOLUME_FADE;
  EventMap[0xe5] = EVENT_PORTAMENTO;
  EventMap[0xe6] = EVENT_PORTAMENTO_TOGGLE;
  EventMap[0xe7] = EVENT_PAN;
  EventMap[0xe8] = EVENT_PAN_FADE;
  EventMap[0xe9] = EVENT_PAN_LFO_ON;
  EventMap[0xea] = EVENT_PAN_LFO_RESTART;
  EventMap[0xeb] = EVENT_PAN_LFO_OFF;
  EventMap[0xec] = EVENT_TRANSPOSE_ABS;
  EventMap[0xed] = EVENT_TRANSPOSE_REL;
  EventMap[0xee] = EVENT_PERC_ON;
  EventMap[0xef] = EVENT_PERC_OFF;
  EventMap[0xf0] = EVENT_VIBRATO_ON;
  EventMap[0xf1] = EVENT_VIBRATO_ON_WITH_DELAY;
  EventMap[0xf2] = EVENT_TEMPO_REL;
  EventMap[0xf3] = EVENT_VIBRATO_OFF;
  EventMap[0xf4] = EVENT_TREMOLO_ON;
  EventMap[0xf5] = EVENT_TREMOLO_ON_WITH_DELAY;
  if (version == SUZUKISNES_SD3) {
    EventMap[0xf6] = EVENT_OCTAVE_UP; // duplicated
  }
  else {
    EventMap[0xf6] = EVENT_UNKNOWN1;
  }
  EventMap[0xf7] = EVENT_TREMOLO_OFF;
  EventMap[0xf8] = EVENT_SLUR_ON;
  EventMap[0xf9] = EVENT_SLUR_OFF;
  EventMap[0xfa] = EVENT_ECHO_ON;
  EventMap[0xfb] = EVENT_ECHO_OFF;
  if (version == SUZUKISNES_SD3) {
    EventMap[0xfc] = EVENT_CALL_SFX_LO;
    EventMap[0xfd] = EVENT_CALL_SFX_HI;
    EventMap[0xfe] = EVENT_OCTAVE_UP; // duplicated
    EventMap[0xff] = EVENT_OCTAVE_UP; // duplicated
  }
  else if (version == SUZUKISNES_BL) {
    EventMap[0xfc] = EVENT_OCTAVE_UP; // duplicated
    EventMap[0xfd] = EVENT_OCTAVE_UP; // duplicated
    EventMap[0xfe] = EVENT_UNKNOWN0;
    EventMap[0xff] = EVENT_UNKNOWN0;
  }
  else if (version == SUZUKISNES_SMR) {
    EventMap[0xfc] = EVENT_UNKNOWN3;
    EventMap[0xfd] = EVENT_OCTAVE_UP; // duplicated
    EventMap[0xfe] = EVENT_UNKNOWN0;
    EventMap[0xff] = EVENT_OCTAVE_UP; // duplicated
  }
}

double SuzukiSnesSeq::GetTempoInBPM(uint8_t tempo) {
  if (tempo != 0) {
    return 60000000. / (125. * tempo * SEQ_PPQN);
  }
  else {
    return 1.0; // since tempo 0 cannot be expressed, this function returns a very small value.
  }
}

//  ***************
//  SuzukiSnesTrack
//  ***************

SuzukiSnesTrack::SuzukiSnesTrack(SuzukiSnesSeq *parentFile, long offset, long length)
    : SeqTrack(parentFile, offset, length) {
}

void SuzukiSnesTrack::ResetVars() {
  SeqTrack::ResetVars();

//  vel = 100;
  octave = 6;
  spcVolume = 100;
  loopLevel = 0;
  infiniteLoopPoint = 0;
}

bool SuzukiSnesTrack::ReadEvent() {
  SuzukiSnesSeq *_parentSeq = dynamic_cast<SuzukiSnesSeq *>(this->parentSeq);

  uint32_t beginOffset = curOffset;
  if (curOffset >= 0x10000) {
    return false;
  }

  uint8_t statusByte = GetByte(curOffset++);
  bool bContinue = true;

  std::wstringstream desc;

  auto eventType = static_cast<SuzukiSnesSeqEventType>(0);
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

    case EVENT_NOTE: // 0x00..0xc3
    {
      uint8_t durIndex = statusByte / 14;
      uint8_t noteIndex = statusByte % 14;

      uint32_t dur;
      if (durIndex == 13) {
        dur = GetByte(curOffset++);
        if (_parentSeq->version == SUZUKISNES_SD3)
          dur++;
      }
      else {
        dur = SuzukiSnesSeq::NOTE_DUR_TABLE[durIndex];
      }

      if (noteIndex < 12) {
        uint8_t note = octave * 12 + noteIndex;

        // TODO: percussion note

        AddNoteByDur(beginOffset, curOffset - beginOffset, note, DEFAULT_VEL, dur);
        AddTime(dur);
      }
      else if (noteIndex == 13) {
        MakePrevDurNoteEnd(GetTime() + dur);
        AddGenericEvent(beginOffset, curOffset - beginOffset, L"Tie", desc.str(), CLR_TIE, ICON_NOTE);
        AddTime(dur);
      }
      else {
        AddRest(beginOffset, curOffset - beginOffset, dur);
      }

      break;
    }

    case EVENT_OCTAVE_UP: {
      AddIncrementOctave(beginOffset, curOffset - beginOffset);
      break;
    }

    case EVENT_OCTAVE_DOWN: {
      AddDecrementOctave(beginOffset, curOffset - beginOffset);
      break;
    }

    case EVENT_OCTAVE: {
      uint8_t newOctave = GetByte(curOffset++);
      AddSetOctave(beginOffset, curOffset - beginOffset, newOctave);
      break;
    }

    case EVENT_NOP: {
      AddGenericEvent(beginOffset, curOffset - beginOffset, L"NOP", desc.str(), CLR_MISC, ICON_BINARY);
      break;
    }

    case EVENT_NOISE_FREQ: {
      uint8_t newNCK = GetByte(curOffset++) & 0x1f;
      desc << L"Noise Frequency (NCK): " << newNCK;
      AddGenericEvent(beginOffset,
                      curOffset - beginOffset,
                      L"Noise Frequency",
                      desc.str(),
                      CLR_CHANGESTATE,
                      ICON_CONTROL);
      break;
    }

    case EVENT_NOISE_ON: {
      AddGenericEvent(beginOffset,
                      curOffset - beginOffset,
                      L"Noise On",
                      desc.str(),
                      CLR_CHANGESTATE,
                      ICON_CONTROL);
      break;
    }

    case EVENT_NOISE_OFF: {
      AddGenericEvent(beginOffset,
                      curOffset - beginOffset,
                      L"Noise Off",
                      desc.str(),
                      CLR_CHANGESTATE,
                      ICON_CONTROL);
      break;
    }

    case EVENT_PITCH_MOD_ON: {
      AddGenericEvent(beginOffset,
                      curOffset - beginOffset,
                      L"Pitch Modulation On",
                      desc.str(),
                      CLR_CHANGESTATE,
                      ICON_CONTROL);
      break;
    }

    case EVENT_PITCH_MOD_OFF: {
      AddGenericEvent(beginOffset,
                      curOffset - beginOffset,
                      L"Pitch Modulation Off",
                      desc.str(),
                      CLR_CHANGESTATE,
                      ICON_CONTROL);
      break;
    }

    case EVENT_JUMP_TO_SFX_LO: {
      // TODO: EVENT_JUMP_TO_SFX_LO
      uint8_t sfxIndex = GetByte(curOffset++);
      desc << L"SFX: " << sfxIndex;
      AddUnknown(beginOffset, curOffset - beginOffset, L"Jump to SFX (LOWORD)", desc.str());
      bContinue = false;
      break;
    }

    case EVENT_JUMP_TO_SFX_HI: {
      // TODO: EVENT_JUMP_TO_SFX_HI
      uint8_t sfxIndex = GetByte(curOffset++);
      desc << L"SFX: " << sfxIndex;
      AddUnknown(beginOffset, curOffset - beginOffset, L"Jump to SFX (HIWORD)", desc.str());
      bContinue = false;
      break;
    }

    case EVENT_CALL_SFX_LO: {
      // TODO: EVENT_CALL_SFX_LO
      uint8_t sfxIndex = GetByte(curOffset++);
      desc << L"SFX: " << sfxIndex;
      AddUnknown(beginOffset, curOffset - beginOffset, L"Call SFX (LOWORD)", desc.str());
      bContinue = false;
      break;
    }

    case EVENT_CALL_SFX_HI: {
      // TODO: EVENT_CALL_SFX_HI
      uint8_t sfxIndex = GetByte(curOffset++);
      desc << L"SFX: " << sfxIndex;
      AddUnknown(beginOffset, curOffset - beginOffset, L"Call SFX (HIWORD)", desc.str());
      bContinue = false;
      break;
    }

    case EVENT_TUNING: {
      int8_t newTuning = GetByte(curOffset++);
      AddFineTuning(beginOffset, curOffset - beginOffset, (newTuning / 16.0) * 100.0);
      break;
    }

    case EVENT_END: {
      // TODO: add "return from SFX" handler
      if ((infiniteLoopPoint & 0xff00) != 0) {
        bContinue = AddLoopForever(beginOffset, curOffset - beginOffset);
        curOffset = infiniteLoopPoint;
      }
      else {
        AddEndOfTrack(beginOffset, curOffset - beginOffset);
        bContinue = false;
      }
      break;
    }

    case EVENT_TEMPO: {
      uint8_t newTempo = GetByte(curOffset++);
      _parentSeq->spcTempo = newTempo;
      AddTempoBPM(beginOffset, curOffset - beginOffset,
                  _parentSeq->GetTempoInBPM(_parentSeq->spcTempo));
      break;
    }

    case EVENT_TEMPO_REL: {
      int8_t delta = GetByte(curOffset++);
      _parentSeq->spcTempo += delta;
      AddTempoBPM(beginOffset,
                  curOffset - beginOffset,
                  _parentSeq->GetTempoInBPM(_parentSeq->spcTempo),
                  L"Tempo (Relative)");
      break;
    }

    case EVENT_TIMER1_FREQ: {
      uint8_t newFreq = GetByte(curOffset++);
      desc << L"Frequency: " << (0.125 * newFreq) << L"ms";
      AddGenericEvent(beginOffset,
                      curOffset - beginOffset,
                      L"Timer 1 Frequency",
                      desc.str(),
                      CLR_CHANGESTATE,
                      ICON_TEMPO);
      break;
    }

    case EVENT_TIMER1_FREQ_REL: {
      int8_t delta = GetByte(curOffset++);
      desc << L"Frequency Delta: " << (0.125 * delta) << L"ms";
      AddGenericEvent(beginOffset,
                      curOffset - beginOffset,
                      L"Timer 1 Frequency (Relative)",
                      desc.str(),
                      CLR_CHANGESTATE,
                      ICON_TEMPO);
      break;
    }

    case EVENT_LOOP_START: {
      uint8_t count = GetByte(curOffset++);
      int realLoopCount = (count == 0) ? 256 : count;

      desc << L"Loop Count: " << realLoopCount;
      AddGenericEvent(beginOffset, curOffset - beginOffset, L"Loop Start", desc.str(), CLR_LOOP, ICON_STARTREP);

      if (loopLevel >= SUZUKISNES_LOOP_LEVEL_MAX) {
        // stack overflow
        break;
      }

      loopStart[loopLevel] = curOffset;
      loopCount[loopLevel] = count - 1;
      loopOctave[loopLevel] = octave;
      loopLevel++;
      break;
    }

    case EVENT_LOOP_END: {
      AddGenericEvent(beginOffset, curOffset - beginOffset, L"Loop End", desc.str(), CLR_LOOP, ICON_ENDREP);

      if (loopLevel == 0) {
        // stack overflow
        break;
      }

      if (loopCount[loopLevel - 1] == 0) {
        // repeat end
        loopLevel--;
      }
      else {
        // repeat again
        octave = loopOctave[loopLevel - 1];
        loopEnd[loopLevel - 1] = curOffset;
        curOffset = loopStart[loopLevel - 1];
        loopCount[loopLevel - 1]--;
      }

      break;
    }

    case EVENT_LOOP_BREAK: {
      AddGenericEvent(beginOffset, curOffset - beginOffset, L"Loop Break", desc.str(), CLR_LOOP, ICON_ENDREP);

      if (loopLevel == 0) {
        // stack overflow
        break;
      }

      if (loopCount[loopLevel - 1] == 0) {
        // repeat end
        curOffset = loopEnd[loopLevel - 1];
        loopLevel--;
      }

      break;
    }

    case EVENT_LOOP_POINT: {
      infiniteLoopPoint = curOffset;
      AddGenericEvent(beginOffset,
                      curOffset - beginOffset,
                      L"Infinite Loop Point",
                      desc.str(),
                      CLR_LOOP,
                      ICON_STARTREP);
      break;
    }

    case EVENT_ADSR_DEFAULT: {
      AddGenericEvent(beginOffset,
                      curOffset - beginOffset,
                      L"Default ADSR",
                      desc.str(),
                      CLR_ADSR,
                      ICON_CONTROL);
      break;
    }

    case EVENT_ADSR_AR: {
      uint8_t newAR = GetByte(curOffset++) & 15;
      desc << L"AR: " << newAR;
      AddGenericEvent(beginOffset,
                      curOffset - beginOffset,
                      L"ADSR Attack Rate",
                      desc.str(),
                      CLR_ADSR,
                      ICON_CONTROL);
      break;
    }

    case EVENT_ADSR_DR: {
      uint8_t newDR = GetByte(curOffset++) & 7;
      desc << L"DR: " << newDR;
      AddGenericEvent(beginOffset,
                      curOffset - beginOffset,
                      L"ADSR Decay Rate",
                      desc.str(),
                      CLR_ADSR,
                      ICON_CONTROL);
      break;
    }

    case EVENT_ADSR_SL: {
      uint8_t newSL = GetByte(curOffset++) & 7;
      desc << L"SL: " << newSL;
      AddGenericEvent(beginOffset,
                      curOffset - beginOffset,
                      L"ADSR Sustain Level",
                      desc.str(),
                      CLR_ADSR,
                      ICON_CONTROL);
      break;
    }

    case EVENT_ADSR_SR: {
      uint8_t newSR = GetByte(curOffset++) & 15;
      desc << L"SR: " << newSR;
      AddGenericEvent(beginOffset,
                      curOffset - beginOffset,
                      L"ADSR Sustain Rate",
                      desc.str(),
                      CLR_ADSR,
                      ICON_CONTROL);
      break;
    }

    case EVENT_DURATION_RATE: {
      // TODO: save duration rate and apply to note length
      uint8_t newDurRate = GetByte(curOffset++);
      desc << L"Duration Rate: " << newDurRate;
      AddGenericEvent(beginOffset, curOffset - beginOffset, L"Duration Rate", desc.str(), CLR_DURNOTE);
      break;
    }

    case EVENT_PROGCHANGE: {
      uint8_t newProg = GetByte(curOffset++);
      AddProgramChange(beginOffset, curOffset - beginOffset, newProg);
      break;
    }

    case EVENT_NOISE_FREQ_REL: {
      int8_t delta = GetByte(curOffset++);
      desc << L"Noise Frequency (NCK) Delta: " << delta;
      AddGenericEvent(beginOffset,
                      curOffset - beginOffset,
                      L"Noise Frequency (Relative)",
                      desc.str(),
                      CLR_CHANGESTATE,
                      ICON_CONTROL);
      break;
    }

    case EVENT_VOLUME: {
      uint8_t _vol = GetByte(curOffset++);
      spcVolume = _vol & 0x7f;
      AddVol(beginOffset, curOffset - beginOffset, spcVolume);
      break;
    }

    case EVENT_VOLUME_REL: {
      int8_t delta = GetByte(curOffset++);
      spcVolume = (spcVolume + delta) & 0x7f;
      AddVol(beginOffset, curOffset - beginOffset, spcVolume, L"Volume (Relative)");
      break;
    }

    case EVENT_VOLUME_FADE: {
      uint8_t fadeLength = GetByte(curOffset++);
      uint8_t _vol = GetByte(curOffset++);
      desc << L"Fade Length: " << fadeLength << L"  Volume: " << _vol;
      AddGenericEvent(beginOffset,
                      curOffset - beginOffset,
                      L"Volume Fade",
                      desc.str(),
                      CLR_VOLUME,
                      ICON_CONTROL);
      break;
    }

    case EVENT_PORTAMENTO: {
      uint8_t arg1 = GetByte(curOffset++);
      uint8_t arg2 = GetByte(curOffset++);
      desc << L"Arg1: " << arg1 << L"  Arg2: " << arg2;
      AddGenericEvent(beginOffset,
                      curOffset - beginOffset,
                      L"Portamento",
                      desc.str(),
                      CLR_PORTAMENTO,
                      ICON_CONTROL);
      break;
    }

    case EVENT_PORTAMENTO_TOGGLE: {
      AddGenericEvent(beginOffset,
                      curOffset - beginOffset,
                      L"Portamento On/Off",
                      desc.str(),
                      CLR_PORTAMENTO,
                      ICON_CONTROL);
      break;
    }

    case EVENT_PAN: {
      // For left pan, the engine will decrease right volume (linear), but will do nothing to left volume.
      // For right pan, the engine will do the opposite.
      // For center pan, it will not decrease any volumes.
      uint8_t pan = GetByte(curOffset++);

      // TODO: correct midi pan value, apply volume scale
      AddPan(beginOffset, curOffset - beginOffset, pan >> 1);
      break;
    }

    case EVENT_PAN_FADE: {
      uint8_t fadeLength = GetByte(curOffset++);
      uint8_t pan = GetByte(curOffset++);
      desc << L"Fade Length: " << fadeLength << L"  Pan: " << (pan >> 1);

      // TODO: correct midi pan value, apply volume scale, do pan slide
      AddGenericEvent(beginOffset, curOffset - beginOffset, L"Pan Fade", desc.str(), CLR_PAN, ICON_CONTROL);
      break;
    }

    case EVENT_PAN_LFO_ON: {
      uint8_t lfoDepth = GetByte(curOffset++);
      uint8_t lfoRate = GetByte(curOffset++);
      desc << L"Depth: " << lfoDepth << L"  Rate: " << lfoRate;
      AddGenericEvent(beginOffset,
                      curOffset - beginOffset,
                      L"Pan LFO",
                      desc.str(),
                      CLR_MODULATION,
                      ICON_CONTROL);
      break;
    }

    case EVENT_PAN_LFO_RESTART: {
      AddGenericEvent(beginOffset,
                      curOffset - beginOffset,
                      L"Pan LFO Restart",
                      desc.str(),
                      CLR_MODULATION,
                      ICON_CONTROL);
      break;
    }

    case EVENT_PAN_LFO_OFF: {
      AddGenericEvent(beginOffset,
                      curOffset - beginOffset,
                      L"Pan LFO Off",
                      desc.str(),
                      CLR_MODULATION,
                      ICON_CONTROL);
      break;
    }

    case EVENT_TRANSPOSE_ABS: {
      // TODO: fraction part of transpose?
      int8_t newTranspose = GetByte(curOffset++);
      int8_t semitones = newTranspose / 4;
      AddTranspose(beginOffset, curOffset - beginOffset, semitones);
      break;
    }

    case EVENT_TRANSPOSE_REL: {
      // TODO: fraction part of transpose?
      int8_t newTranspose = GetByte(curOffset++);
      int8_t semitones = newTranspose / 4;
      AddTranspose(beginOffset, curOffset - beginOffset, transpose + semitones, L"Transpose (Relative)");
      break;
    }

    case EVENT_PERC_ON: {
      AddGenericEvent(beginOffset,
                      curOffset - beginOffset,
                      L"Percussion On",
                      desc.str(),
                      CLR_CHANGESTATE,
                      ICON_CONTROL);
      break;
    }

    case EVENT_PERC_OFF: {
      AddGenericEvent(beginOffset,
                      curOffset - beginOffset,
                      L"Percussion Off",
                      desc.str(),
                      CLR_CHANGESTATE,
                      ICON_CONTROL);
      break;
    }

    case EVENT_VIBRATO_ON: {
      uint8_t lfoDepth = GetByte(curOffset++);
      uint8_t lfoRate = GetByte(curOffset++);
      desc << L"Depth: " << lfoDepth << L"  Rate: " << lfoRate;
      AddGenericEvent(beginOffset,
                      curOffset - beginOffset,
                      L"Vibrato",
                      desc.str(),
                      CLR_MODULATION,
                      ICON_CONTROL);
      break;
    }

    case EVENT_VIBRATO_ON_WITH_DELAY: {
      uint8_t lfoDepth = GetByte(curOffset++);
      uint8_t lfoRate = GetByte(curOffset++);
      uint8_t lfoDelay = GetByte(curOffset++);
      desc << L"Depth: " << lfoDepth << L"  Rate: " << lfoRate << L"  Delay: " << lfoDelay;
      AddGenericEvent(beginOffset,
                      curOffset - beginOffset,
                      L"Vibrato",
                      desc.str(),
                      CLR_MODULATION,
                      ICON_CONTROL);
      break;
    }

    case EVENT_VIBRATO_OFF: {
      AddGenericEvent(beginOffset,
                      curOffset - beginOffset,
                      L"Vibrato Off",
                      desc.str(),
                      CLR_MODULATION,
                      ICON_CONTROL);
      break;
    }

    case EVENT_TREMOLO_ON: {
      uint8_t lfoDepth = GetByte(curOffset++);
      uint8_t lfoRate = GetByte(curOffset++);
      desc << L"Depth: " << lfoDepth << L"  Rate: " << lfoRate;
      AddGenericEvent(beginOffset,
                      curOffset - beginOffset,
                      L"Tremolo",
                      desc.str(),
                      CLR_MODULATION,
                      ICON_CONTROL);
      break;
    }

    case EVENT_TREMOLO_ON_WITH_DELAY: {
      uint8_t lfoDepth = GetByte(curOffset++);
      uint8_t lfoRate = GetByte(curOffset++);
      uint8_t lfoDelay = GetByte(curOffset++);
      desc << L"Depth: " << lfoDepth << L"  Rate: " << lfoRate << L"  Delay: " << lfoDelay;
      AddGenericEvent(beginOffset,
                      curOffset - beginOffset,
                      L"Tremolo",
                      desc.str(),
                      CLR_MODULATION,
                      ICON_CONTROL);
      break;
    }

    case EVENT_TREMOLO_OFF: {
      AddGenericEvent(beginOffset,
                      curOffset - beginOffset,
                      L"Tremolo Off",
                      desc.str(),
                      CLR_MODULATION,
                      ICON_CONTROL);
      break;
    }

    case EVENT_SLUR_ON: {
      AddGenericEvent(beginOffset,
                      curOffset - beginOffset,
                      L"Slur On",
                      desc.str(),
                      CLR_CHANGESTATE,
                      ICON_CONTROL);
      break;
    }

    case EVENT_SLUR_OFF: {
      AddGenericEvent(beginOffset,
                      curOffset - beginOffset,
                      L"Slur Off",
                      desc.str(),
                      CLR_CHANGESTATE,
                      ICON_CONTROL);
      break;
    }

    case EVENT_ECHO_ON: {
      AddGenericEvent(beginOffset, curOffset - beginOffset, L"Echo On", desc.str(), CLR_REVERB, ICON_CONTROL);
      break;
    }

    case EVENT_ECHO_OFF: {
      AddGenericEvent(beginOffset, curOffset - beginOffset, L"Echo Off", desc.str(), CLR_REVERB, ICON_CONTROL);
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
