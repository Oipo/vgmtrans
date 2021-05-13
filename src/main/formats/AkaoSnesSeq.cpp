#include "pch.h"
#include "AkaoSnesSeq.h"

#include <utility>
#include "AkaoSnesInstr.h"
#include "ScaleConversion.h"

DECLARE_FORMAT(AkaoSnes);

//  **********
//  AkaoSnesSeq
//  **********
#define MAX_TRACKS  8
#define SEQ_PPQN    48

AkaoSnesSeq::AkaoSnesSeq(RawFile *file,
                         AkaoSnesVersion ver,
                         AkaoSnesMinorVersion minorVer,
                         uint32_t seqdataOffset,
                         uint32_t _addrAPURelocBase,
                         std::wstring newName)
    : VGMSeq(AkaoSnesFormat::name, file, seqdataOffset, 0, std::move(newName)),
      version(ver),
      minorVersion(minorVer),
      addrAPURelocBase(_addrAPURelocBase),
      addrROMRelocBase(_addrAPURelocBase),
      addrSequenceEnd(0) {
  bLoadTickByTick = true;
  bAllowDiscontinuousTrackData = true;
  bUseLinearAmplitudeScale = true;

  UseReverb();
  AlwaysWriteInitialReverb(0);

  LoadEventMap();
}

AkaoSnesSeq::~AkaoSnesSeq() = default;

void AkaoSnesSeq::ResetVars() {
  VGMSeq::ResetVars();
}

bool AkaoSnesSeq::GetHeaderInfo() {
  SetPPQN(SEQ_PPQN);

  VGMHeader *header = AddHeader(dwOffset, 0);
  uint32_t curOffset = dwOffset;
  if (version == AKAOSNES_V1 || version == AKAOSNES_V2) {
    // Earlier versions are not relocatable
    // All addresses are plain APU addresses
    addrROMRelocBase = addrAPURelocBase;
    addrSequenceEnd = ROMAddressToAPUAddress(0);
  }
  else {
    // Later versions are relocatable
    if (version == AKAOSNES_V3) {
      header->AddSimpleItem(curOffset, 2, L"ROM Address Base");
      addrROMRelocBase = GetShort(curOffset);
      if (minorVersion != AKAOSNES_V3_FFMQ) {
        curOffset += 2;
      }

      header->AddSimpleItem(curOffset + MAX_TRACKS * 2, 2, L"End Address");
      addrSequenceEnd = GetShortAddress(curOffset + MAX_TRACKS * 2);
    }
    else if (version == AKAOSNES_V4) {
      header->AddSimpleItem(curOffset, 2, L"ROM Address Base");
      addrROMRelocBase = GetShort(curOffset);
      curOffset += 2;

      header->AddSimpleItem(curOffset, 2, L"End Address");
      addrSequenceEnd = GetShortAddress(curOffset);
      curOffset += 2;
    }

    // calculate sequence length
    if (addrSequenceEnd < dwOffset) {
      return false;
    }
    unLength = addrSequenceEnd - dwOffset;
  }

  for (uint8_t trackIndex = 0; trackIndex < MAX_TRACKS; trackIndex++) {
    uint16_t addrTrackStart = GetShortAddress(curOffset);
    if (addrTrackStart != addrSequenceEnd) {
      std::wstringstream trackName;
      trackName << L"Track Pointer " << (trackIndex + 1);
      header->AddSimpleItem(curOffset, 2, trackName.str());
    }
    else {
      header->AddSimpleItem(curOffset, 2, L"nullptr");
    }
    curOffset += 2;
  }

  header->SetGuessedLength();

  return true;
}


bool AkaoSnesSeq::GetTrackPointers() {
  uint32_t curOffset = dwOffset;
  if (version == AKAOSNES_V3) {
    if (minorVersion != AKAOSNES_V3_FFMQ) {
      curOffset += 2;
    }
  }
  else if (version == AKAOSNES_V4) {
    curOffset += 4;
  }

  for (uint8_t trackIndex = 0; trackIndex < MAX_TRACKS; trackIndex++) {
    uint16_t addrTrackStart = GetShortAddress(curOffset);
    if (addrTrackStart != addrSequenceEnd) {
      AkaoSnesTrack *track = new AkaoSnesTrack(this, addrTrackStart);
      aTracks.push_back(track);
    }
    curOffset += 2;
  }

  return true;
}

void AkaoSnesSeq::LoadEventMap() {
  int statusByte;

  // opcode min/max
  switch (version) {
    case AKAOSNES_V1:
      STATUS_NOTE_MAX = 0xd1;
      STATUS_NOTEINDEX_REST = 12;
      STATUS_NOTEINDEX_TIE = 13;
      break;

    case AKAOSNES_V2:
      STATUS_NOTE_MAX = 0xd1;
      STATUS_NOTEINDEX_REST = 12;
      STATUS_NOTEINDEX_TIE = 13;
      break;

    case AKAOSNES_V3:
      STATUS_NOTE_MAX = 0xd1;
      STATUS_NOTEINDEX_TIE = 12;
      STATUS_NOTEINDEX_REST = 13;
      break;

    case AKAOSNES_V4:
      STATUS_NOTE_MAX = 0xc3;
      STATUS_NOTEINDEX_TIE = 12;
      STATUS_NOTEINDEX_REST = 13;
      break;
    case AKAOSNES_NONE:
      break;
  }

  // duration table
  if (version == AKAOSNES_V1) {
    const uint8_t NOTE_DUR_TABLE_FF4[15] = {
        0xc0, 0x90, 0x60, 0x48, 0x40, 0x30, 0x24, 0x20, 0x18, 0x10, 0x0c, 0x08, 0x06, 0x04, 0x03
    };
    NOTE_DUR_TABLE.assign(std::begin(NOTE_DUR_TABLE_FF4), std::end(NOTE_DUR_TABLE_FF4));
  }
  else if (version == AKAOSNES_V2 || version == AKAOSNES_V3) {
    const uint8_t NOTE_DUR_TABLE_RS1[15] = {
        0xc0, 0x90, 0x60, 0x40, 0x48, 0x30, 0x20, 0x24, 0x18, 0x10, 0x0c, 0x08, 0x06, 0x04, 0x03
    };
    NOTE_DUR_TABLE.assign(std::begin(NOTE_DUR_TABLE_RS1), std::end(NOTE_DUR_TABLE_RS1));
  }
  else { // AKAOSNES_V4
    const uint8_t NOTE_DUR_TABLE_RS2[14] = {
        0xc0, 0x60, 0x40, 0x48, 0x30, 0x20, 0x24, 0x18, 0x10, 0x0c, 0x08, 0x06, 0x04, 0x03
    };
    NOTE_DUR_TABLE.assign(std::begin(NOTE_DUR_TABLE_RS2), std::end(NOTE_DUR_TABLE_RS2));
  }

  // timer 0 frequency
  if (version == AKAOSNES_V4) {
    if (minorVersion == AKAOSNES_V4_RS2 || minorVersion == AKAOSNES_V4_LAL) {
      TIMER0_FREQUENCY = 0x24;
    }
    else if (minorVersion == AKAOSNES_V4_FM || minorVersion == AKAOSNES_V4_CT) {
      TIMER0_FREQUENCY = 0x2a;
    }
    else {
      TIMER0_FREQUENCY = 0x27;
    }
  }
  else { // AKAOSNES_V1, AKAOSNES_V2, AKAOSNES_V3
    TIMER0_FREQUENCY = 0x24;
  }

  // pan bitdepth
  if (version == AKAOSNES_V1 || version == AKAOSNES_V3) {
    PAN_8BIT = true;
  }
  else if (version == AKAOSNES_V2) {
    PAN_8BIT = false;
  }
  else { // AKAOSNES_V4
    if (minorVersion == AKAOSNES_V4_RS2 || minorVersion == AKAOSNES_V4_LAL) {
      PAN_8BIT = true;
    }
    else {
      PAN_8BIT = false;
    }
  }

  for (statusByte = 0x00; statusByte <= STATUS_NOTE_MAX; statusByte++) {
    EventMap[statusByte] = EVENT_NOTE;
  }

  if (version == AKAOSNES_V1) {
    EventMap[0xd2] = EVENT_TEMPO_FADE;
    EventMap[0xd3] = EVENT_NOP1;
    EventMap[0xd4] = EVENT_ECHO_VOLUME;
    EventMap[0xd5] = EVENT_ECHO_FEEDBACK_FIR;
    EventMap[0xd6] = EVENT_PITCH_SLIDE_ON;
    EventMap[0xd7] = EVENT_TREMOLO_ON;
    EventMap[0xd8] = EVENT_VIBRATO_ON;
    EventMap[0xd9] = EVENT_PAN_LFO_ON_WITH_DELAY;
    EventMap[0xda] = EVENT_OCTAVE;
    EventMap[0xdb] = EVENT_PROGCHANGE;
    EventMap[0xdc] = EVENT_VOLUME_ENVELOPE;
    EventMap[0xdd] = EVENT_GAIN_RELEASE;
    EventMap[0xde] = EVENT_DURATION_RATE;
    EventMap[0xdf] = EVENT_NOISE_FREQ;
    EventMap[0xe0] = EVENT_LOOP_START;
    EventMap[0xe1] = EVENT_OCTAVE_UP;
    EventMap[0xe2] = EVENT_OCTAVE_DOWN;
    EventMap[0xe3] = EVENT_NOP;
    EventMap[0xe4] = EVENT_NOP;
    EventMap[0xe5] = EVENT_NOP;
    EventMap[0xe6] = EVENT_PITCH_SLIDE_OFF;
    EventMap[0xe7] = EVENT_TREMOLO_OFF;
    EventMap[0xe8] = EVENT_VIBRATO_OFF;
    EventMap[0xe9] = EVENT_PAN_LFO_OFF;
    EventMap[0xea] = EVENT_ECHO_ON;
    EventMap[0xeb] = EVENT_ECHO_OFF;
    EventMap[0xec] = EVENT_NOISE_ON;
    EventMap[0xed] = EVENT_NOISE_OFF;
    EventMap[0xee] = EVENT_PITCHMOD_ON;
    EventMap[0xef] = EVENT_PITCHMOD_OFF;
    EventMap[0xf0] = EVENT_LOOP_END;
    EventMap[0xf1] = EVENT_END;
    EventMap[0xf2] = EVENT_VOLUME_FADE;
    EventMap[0xf3] = EVENT_PAN_FADE;
    EventMap[0xf4] = EVENT_GOTO;
    EventMap[0xf5] = EVENT_LOOP_BREAK;
    EventMap[0xf6] = EVENT_UNKNOWN0; // cpu-controled jump?
    EventMap[0xf7] = EVENT_END; // duplicated
    EventMap[0xf8] = EVENT_END; // duplicated
    EventMap[0xf9] = EVENT_END; // duplicated
    EventMap[0xfa] = EVENT_END; // duplicated
    EventMap[0xfb] = EVENT_END; // duplicated
    EventMap[0xfc] = EVENT_END; // duplicated
    EventMap[0xfd] = EVENT_END; // duplicated
    EventMap[0xfe] = EVENT_END; // duplicated
    EventMap[0xff] = EVENT_END; // duplicated
  }
  else if (version == AKAOSNES_V2) {
    EventMap[0xd2] = EVENT_TEMPO;
    EventMap[0xd3] = EVENT_TEMPO_FADE;
    EventMap[0xd4] = EVENT_VOLUME;
    EventMap[0xd5] = EVENT_VOLUME_FADE;
    EventMap[0xd6] = EVENT_PAN;
    EventMap[0xd7] = EVENT_PAN_FADE;
    EventMap[0xd8] = EVENT_ECHO_VOLUME;
    EventMap[0xd9] = EVENT_ECHO_VOLUME_FADE;
    EventMap[0xda] = EVENT_TRANSPOSE_ABS;
    EventMap[0xdb] = EVENT_PITCH_SLIDE_ON;
    EventMap[0xdc] = EVENT_PITCH_SLIDE_OFF;
    EventMap[0xdd] = EVENT_TREMOLO_ON;
    EventMap[0xde] = EVENT_TREMOLO_OFF;
    EventMap[0xdf] = EVENT_VIBRATO_ON;
    EventMap[0xe0] = EVENT_VIBRATO_OFF;
    EventMap[0xe1] = EVENT_NOISE_FREQ;
    EventMap[0xe2] = EVENT_NOISE_ON;
    EventMap[0xe3] = EVENT_NOISE_OFF;
    EventMap[0xe4] = EVENT_PITCHMOD_ON;
    EventMap[0xe5] = EVENT_PITCHMOD_OFF;
    EventMap[0xe6] = EVENT_ECHO_FEEDBACK_FIR;
    EventMap[0xe7] = EVENT_ECHO_ON;
    EventMap[0xe8] = EVENT_ECHO_OFF;
    EventMap[0xe9] = EVENT_PAN_LFO_ON;
    EventMap[0xea] = EVENT_PAN_LFO_OFF;
    EventMap[0xeb] = EVENT_OCTAVE;
    EventMap[0xec] = EVENT_OCTAVE_UP;
    EventMap[0xed] = EVENT_OCTAVE_DOWN;
    EventMap[0xee] = EVENT_LOOP_START;
    EventMap[0xef] = EVENT_LOOP_END;
    EventMap[0xf0] = EVENT_LOOP_BREAK;
    EventMap[0xf1] = EVENT_GOTO;
    EventMap[0xf2] = EVENT_SLUR_ON;
    EventMap[0xf3] = EVENT_PROGCHANGE;
    EventMap[0xf4] = EVENT_VOLUME_ENVELOPE;
    EventMap[0xf5] = EVENT_SLUR_OFF;
    EventMap[0xf6] = EVENT_UNKNOWN2; // cpu-controled jump?
    EventMap[0xf7] = EVENT_TUNING;
    EventMap[0xf8] = EVENT_END; // duplicated
    EventMap[0xf9] = EVENT_END; // duplicated
    EventMap[0xfa] = EVENT_END; // duplicated
    EventMap[0xfb] = EVENT_END; // duplicated
    EventMap[0xfc] = EVENT_END; // duplicated
    EventMap[0xfd] = EVENT_END; // duplicated
    EventMap[0xfe] = EVENT_END; // duplicated
    EventMap[0xff] = EVENT_END; // duplicated
  }
  else if (version == AKAOSNES_V3) {
    // V3-V4 common
    EventMap[0xd2] = EVENT_VOLUME;
    EventMap[0xd3] = EVENT_VOLUME_FADE;
    EventMap[0xd4] = EVENT_PAN;
    EventMap[0xd5] = EVENT_PAN_FADE;
    EventMap[0xd6] = EVENT_PITCH_SLIDE;
    EventMap[0xd7] = EVENT_VIBRATO_ON;
    EventMap[0xd8] = EVENT_VIBRATO_OFF;
    EventMap[0xd9] = EVENT_TREMOLO_ON;
    EventMap[0xda] = EVENT_TREMOLO_OFF;
    EventMap[0xdb] = EVENT_PAN_LFO_ON;
    EventMap[0xdc] = EVENT_PAN_LFO_OFF;
    EventMap[0xdd] = EVENT_NOISE_FREQ;
    EventMap[0xde] = EVENT_NOISE_ON;
    EventMap[0xdf] = EVENT_NOISE_OFF;
    EventMap[0xe0] = EVENT_PITCHMOD_ON;
    EventMap[0xe1] = EVENT_PITCHMOD_OFF;
    EventMap[0xe2] = EVENT_ECHO_ON;
    EventMap[0xe3] = EVENT_ECHO_OFF;
    EventMap[0xe4] = EVENT_OCTAVE;
    EventMap[0xe5] = EVENT_OCTAVE_UP;
    EventMap[0xe6] = EVENT_OCTAVE_DOWN;
    EventMap[0xe7] = EVENT_TRANSPOSE_ABS;
    EventMap[0xe8] = EVENT_TRANSPOSE_REL;
    EventMap[0xe9] = EVENT_TUNING;
    EventMap[0xea] = EVENT_PROGCHANGE;
    EventMap[0xeb] = EVENT_ADSR_AR;
    EventMap[0xec] = EVENT_ADSR_DR;
    EventMap[0xed] = EVENT_ADSR_SL;
    EventMap[0xee] = EVENT_ADSR_SR;
    EventMap[0xef] = EVENT_ADSR_DEFAULT;
    EventMap[0xf0] = EVENT_LOOP_START;
    EventMap[0xf1] = EVENT_LOOP_END;

    // V3 common
    EventMap[0xf2] = EVENT_END;
    EventMap[0xf3] = EVENT_TEMPO;
    EventMap[0xf4] = EVENT_TEMPO_FADE;
    EventMap[0xf5] = EVENT_ECHO_VOLUME; // No Operation in Seiken Densetsu 2
    EventMap[0xf6] = EVENT_ECHO_VOLUME_FADE; // No Operation in Seiken Densetsu 2
    EventMap[0xf7] = EVENT_ECHO_FEEDBACK_FIR; // No Operation in Seiken Densetsu 2
    EventMap[0xf8] = EVENT_MASTER_VOLUME;
    EventMap[0xf9] = EVENT_LOOP_BREAK;
    EventMap[0xfa] = EVENT_GOTO;
    EventMap[0xfb] = EVENT_CPU_CONTROLED_JUMP;

    // game-specific mappings
    if (minorVersion == AKAOSNES_V3_SD2) {
      EventMap[0xfc] = EVENT_LOOP_RESTART;
      EventMap[0xfd] = EVENT_IGNORE_MASTER_VOLUME_BY_PROGNUM;
      EventMap[0xfe] = EVENT_END;
      EventMap[0xff] = EVENT_END;
    }
    else {
      EventMap[0xfc] = EVENT_END;
      EventMap[0xfd] = EVENT_END;
      EventMap[0xfe] = EVENT_END;
      EventMap[0xff] = EVENT_END;
    }
  }
  else if (version == AKAOSNES_V4) {
    // V3-V4 common
    EventMap[0xc4] = EVENT_VOLUME;
    EventMap[0xc5] = EVENT_VOLUME_FADE;
    EventMap[0xc6] = EVENT_PAN;
    EventMap[0xc7] = EVENT_PAN_FADE;
    EventMap[0xc8] = EVENT_PITCH_SLIDE;
    EventMap[0xc9] = EVENT_VIBRATO_ON;
    EventMap[0xca] = EVENT_VIBRATO_OFF;
    EventMap[0xcb] = EVENT_TREMOLO_ON;
    EventMap[0xcc] = EVENT_TREMOLO_OFF;
    EventMap[0xcd] = EVENT_PAN_LFO_ON;
    EventMap[0xce] = EVENT_PAN_LFO_OFF;
    EventMap[0xcf] = EVENT_NOISE_FREQ;
    EventMap[0xd0] = EVENT_NOISE_ON;
    EventMap[0xd1] = EVENT_NOISE_OFF;
    EventMap[0xd2] = EVENT_PITCHMOD_ON;
    EventMap[0xd3] = EVENT_PITCHMOD_OFF;
    EventMap[0xd4] = EVENT_ECHO_ON;
    EventMap[0xd5] = EVENT_ECHO_OFF;
    EventMap[0xd6] = EVENT_OCTAVE;
    EventMap[0xd7] = EVENT_OCTAVE_UP;
    EventMap[0xd8] = EVENT_OCTAVE_DOWN;
    EventMap[0xd9] = EVENT_TRANSPOSE_ABS;
    EventMap[0xda] = EVENT_TRANSPOSE_REL;
    EventMap[0xdb] = EVENT_TUNING;
    EventMap[0xdc] = EVENT_PROGCHANGE;
    EventMap[0xdd] = EVENT_ADSR_AR;
    EventMap[0xde] = EVENT_ADSR_DR;
    EventMap[0xdf] = EVENT_ADSR_SL;
    EventMap[0xe0] = EVENT_ADSR_SR;
    EventMap[0xe1] = EVENT_ADSR_DEFAULT;
    EventMap[0xe2] = EVENT_LOOP_START;
    EventMap[0xe3] = EVENT_LOOP_END;

    // V4 common
    EventMap[0xe4] = EVENT_SLUR_ON;
    EventMap[0xe5] = EVENT_SLUR_OFF;
    EventMap[0xe6] = EVENT_LEGATO_ON;
    EventMap[0xe7] = EVENT_LEGATO_OFF;
    EventMap[0xe8] = EVENT_ONETIME_DURATION;
    EventMap[0xe9] = EVENT_JUMP_TO_SFX_LO;
    EventMap[0xea] = EVENT_JUMP_TO_SFX_HI;
    EventMap[0xeb] = EVENT_END; // exception: Gun Hazard
    EventMap[0xec] = EVENT_END; // duplicated
    EventMap[0xed] = EVENT_END; // duplicated
    EventMap[0xee] = EVENT_END; // duplicated
    EventMap[0xef] = EVENT_END; // duplicated
    EventMap[0xf0] = EVENT_TEMPO;
    EventMap[0xf1] = EVENT_TEMPO_FADE;
    EventMap[0xf2] = EVENT_ECHO_VOLUME;
    EventMap[0xf3] = EVENT_ECHO_VOLUME_FADE;

    // game-specific mappings
    if (minorVersion == AKAOSNES_V4_RS2) {
      EventMap[0xf4] = EVENT_ECHO_FEEDBACK_FIR;
      EventMap[0xf5] = EVENT_MASTER_VOLUME;
      EventMap[0xf6] = EVENT_LOOP_BREAK;
      EventMap[0xf7] = EVENT_GOTO;
      EventMap[0xf8] = EVENT_INC_CPU_SHARED_COUNTER;
      EventMap[0xf9] = EVENT_ZERO_CPU_SHARED_COUNTER;
      EventMap[0xfa] = EVENT_IGNORE_MASTER_VOLUME_BROKEN;
      EventMap[0xfb] = EVENT_END; // duplicated
      EventMap[0xfc] = EVENT_END; // duplicated
      EventMap[0xfd] = EVENT_END; // duplicated
      EventMap[0xfe] = EVENT_END; // duplicated
      EventMap[0xff] = EVENT_END; // duplicated
    }
    else if (minorVersion == AKAOSNES_V4_LAL) {
      EventMap[0xf4] = EVENT_ECHO_FEEDBACK_FIR;
      EventMap[0xf5] = EVENT_MASTER_VOLUME;
      EventMap[0xf6] = EVENT_LOOP_BREAK;
      EventMap[0xf7] = EVENT_GOTO;
      EventMap[0xf8] = EVENT_INC_CPU_SHARED_COUNTER;
      EventMap[0xf9] = EVENT_ZERO_CPU_SHARED_COUNTER;
      EventMap[0xfa] = EVENT_IGNORE_MASTER_VOLUME;
      EventMap[0xfb] = EVENT_CPU_CONTROLED_JUMP;
      EventMap[0xfc] = EVENT_END; // duplicated
      EventMap[0xfd] = EVENT_END; // duplicated
      EventMap[0xfe] = EVENT_END; // duplicated
      EventMap[0xff] = EVENT_END; // duplicated
    }
    else if (minorVersion == AKAOSNES_V4_FF6) {
      EventMap[0xf4] = EVENT_MASTER_VOLUME;
      EventMap[0xf5] = EVENT_LOOP_BREAK;
      EventMap[0xf6] = EVENT_GOTO;
      EventMap[0xf7] = EVENT_ECHO_FEEDBACK_FADE;
      EventMap[0xf8] = EVENT_ECHO_FIR_FADE;
      EventMap[0xf9] = EVENT_INC_CPU_SHARED_COUNTER;
      EventMap[0xfa] = EVENT_ZERO_CPU_SHARED_COUNTER;
      EventMap[0xfb] = EVENT_IGNORE_MASTER_VOLUME;
      EventMap[0xfc] = EVENT_CPU_CONTROLED_JUMP;
      EventMap[0xfd] = EVENT_END; // duplicated
      EventMap[0xfe] = EVENT_END; // duplicated
      EventMap[0xff] = EVENT_END; // duplicated
    }
    else if (minorVersion == AKAOSNES_V4_FM) {
      EventMap[0xf4] = EVENT_MASTER_VOLUME;
      EventMap[0xf5] = EVENT_LOOP_BREAK;
      EventMap[0xf6] = EVENT_GOTO;
      EventMap[0xf7] = EVENT_ECHO_FEEDBACK_FADE;
      EventMap[0xf8] = EVENT_ECHO_FIR_FADE;
      EventMap[0xf9] = EVENT_UNKNOWN1;
      EventMap[0xfa] = EVENT_CPU_CONTROLED_JUMP;
      EventMap[0xfb] = EVENT_PERC_ON;
      EventMap[0xfc] = EVENT_PERC_OFF;
      EventMap[0xfd] = EVENT_VOLUME_ALT;
      EventMap[0xfe] = EVENT_END; // duplicated
      EventMap[0xff] = EVENT_END; // duplicated
    }
    else if (minorVersion == AKAOSNES_V4_CT) {
      EventMap[0xf4] = EVENT_MASTER_VOLUME;
      EventMap[0xf5] = EVENT_LOOP_BREAK;
      EventMap[0xf6] = EVENT_GOTO;
      EventMap[0xf7] = EVENT_ECHO_FEEDBACK_FADE;
      EventMap[0xf8] = EVENT_ECHO_FIR_FADE;
      EventMap[0xf9] = EVENT_CPU_CONTROLED_SET_VALUE;
      EventMap[0xfa] = EVENT_CPU_CONTROLED_JUMP;
      EventMap[0xfb] = EVENT_PERC_ON;
      EventMap[0xfc] = EVENT_PERC_OFF;
      EventMap[0xfd] = EVENT_VOLUME_ALT;
      EventMap[0xfe] = EVENT_END; // duplicated
      EventMap[0xff] = EVENT_END; // duplicated
    }
    else if (minorVersion == AKAOSNES_V4_RS3) {
      EventMap[0xf4] = EVENT_VOLUME_ALT;
      EventMap[0xf5] = EVENT_LOOP_BREAK;
      EventMap[0xf6] = EVENT_GOTO;
      EventMap[0xf7] = EVENT_ECHO_FEEDBACK;
      EventMap[0xf8] = EVENT_ECHO_FIR;
      EventMap[0xf9] = EVENT_CPU_CONTROLED_SET_VALUE;
      EventMap[0xfa] = EVENT_CPU_CONTROLED_JUMP;
      EventMap[0xfb] = EVENT_PERC_ON;
      EventMap[0xfc] = EVENT_PERC_OFF;
      EventMap[0xfd] = EVENT_PLAY_SFX;
      EventMap[0xfe] = EVENT_END; // duplicated
      EventMap[0xff] = EVENT_END; // duplicated
    }
    else if (minorVersion == AKAOSNES_V4_GH) {
      EventMap[0xeb] = EVENT_UNKNOWN1;
      EventMap[0xec] = EVENT_END;

      EventMap[0xf4] = EVENT_VOLUME_ALT;
      EventMap[0xf5] = EVENT_LOOP_BREAK;
      EventMap[0xf6] = EVENT_GOTO;
      EventMap[0xf7] = EVENT_ECHO_FEEDBACK;
      EventMap[0xf8] = EVENT_ECHO_FIR;
      EventMap[0xf9] = EVENT_CPU_CONTROLED_SET_VALUE;
      EventMap[0xfa] = EVENT_CPU_CONTROLED_JUMP;
      EventMap[0xfb] = EVENT_PERC_ON;
      EventMap[0xfc] = EVENT_PERC_OFF;
      EventMap[0xfd] = EVENT_END; // duplicated
      EventMap[0xfe] = EVENT_END; // duplicated
      EventMap[0xff] = EVENT_END; // duplicated
    }
    else if (minorVersion == AKAOSNES_V4_BSGAME) {
      EventMap[0xf4] = EVENT_VOLUME_ALT;
      EventMap[0xf5] = EVENT_LOOP_BREAK;
      EventMap[0xf6] = EVENT_GOTO;
      EventMap[0xf7] = EVENT_ECHO_FEEDBACK;
      EventMap[0xf8] = EVENT_ECHO_FIR;
      EventMap[0xf9] = EVENT_CPU_CONTROLED_SET_VALUE;
      EventMap[0xfa] = EVENT_CPU_CONTROLED_JUMP;
      EventMap[0xfb] = EVENT_PERC_ON;
      EventMap[0xfc] = EVENT_PERC_OFF;
      EventMap[0xfd] = EVENT_UNKNOWN1;
      EventMap[0xfe] = EVENT_UNKNOWN0;
      EventMap[0xff] = EVENT_END; // duplicated
    }
  }
}

double AkaoSnesSeq::GetTempoInBPM(uint8_t tempo) const {
  if (tempo != 0 && TIMER0_FREQUENCY != 0) {
    return 60000000.0 / (SEQ_PPQN * (125 * TIMER0_FREQUENCY)) * (tempo / 256.0);
  }
  else {
    // since tempo 0 cannot be expressed, this function returns a very small value.
    return 1.0;
  }
}

uint16_t AkaoSnesSeq::ROMAddressToAPUAddress(uint16_t romAddress) const {
  return (romAddress - addrROMRelocBase) + addrAPURelocBase;
}

uint16_t AkaoSnesSeq::GetShortAddress(uint32_t offset) {
  return ROMAddressToAPUAddress(GetShort(offset));
}

//  ************
//  AkaoSnesTrack
//  ************

AkaoSnesTrack::AkaoSnesTrack(AkaoSnesSeq *parentFile, long offset, long length)
    : SeqTrack(parentFile, offset, length) {
  ResetVars();
  bDetermineTrackLengthEventByEvent = true;
  bWriteGenericEventAsTextEvent = false;
}

void AkaoSnesTrack::ResetVars() {
  SeqTrack::ResetVars();

//  vel = 100;
  octave = 6;
  onetimeDuration = 0;
  loopLevel = 0;
  slur = false;
  legato = false;
  percussion = false;
  nonPercussionProgram = 0;

  ignoreMasterVolumeProgNum = 0xff;
}


bool AkaoSnesTrack::ReadEvent() {
  auto *_parentSeq = dynamic_cast<AkaoSnesSeq *>(this->parentSeq);

  uint32_t beginOffset = curOffset;
  if (curOffset >= 0x10000) {
    return false;
  }

  uint8_t statusByte = GetByte(curOffset++);
  bool bContinue = true;

  std::wstringstream desc;

  auto eventType = static_cast<AkaoSnesSeqEventType>(0);
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
      uint8_t durIndex = statusByte % _parentSeq->NOTE_DUR_TABLE.size();
      uint8_t noteIndex = statusByte / _parentSeq->NOTE_DUR_TABLE.size();
      uint8_t len = _parentSeq->NOTE_DUR_TABLE[durIndex];

      if (onetimeDuration != 0) {
        len = onetimeDuration;
        onetimeDuration = 0;
      }

      uint8_t dur = len;
      if (!slur && !legato) {
        dur = (dur > 2) ? dur - 2 : 1;
      }

      if (noteIndex < 12) {
        uint8_t note = octave * 12 + noteIndex;

        if (percussion) {
          AddNoteByDur(beginOffset, curOffset - beginOffset, noteIndex + AkaoSnesDrumKitRgn::KEY_BIAS - transpose, DEFAULT_VEL, dur, L"Percussion Note with Duration");
        }
        else {
          AddNoteByDur(beginOffset, curOffset - beginOffset, note, DEFAULT_VEL, dur);
        }

        AddTime(len);
      }
      else if (noteIndex == _parentSeq->STATUS_NOTEINDEX_TIE) {
        MakePrevDurNoteEnd(GetTime() + dur);
        AddGenericEvent(beginOffset, curOffset - beginOffset, L"Tie", desc.str(), CLR_TIE, ICON_NOTE);
        AddTime(len);
      }
      else {
        AddRest(beginOffset, curOffset - beginOffset, len);
      }

      break;
    }

    case EVENT_NOP: {
      AddGenericEvent(beginOffset, curOffset - beginOffset, L"NOP", desc.str(), CLR_MISC, ICON_BINARY);
      break;
    }

    case EVENT_NOP1: {
      curOffset++;
      AddGenericEvent(beginOffset, curOffset - beginOffset, L"NOP", desc.str(), CLR_MISC, ICON_BINARY);
      break;
    }

    case EVENT_VOLUME: {
      // TODO: correct volume bitdepth?
      uint8_t _vol = GetByte(curOffset++);
      AddVol(beginOffset, curOffset - beginOffset, _vol >> 1);
      break;
    }

    case EVENT_VOLUME_FADE: {
      // TODO: correct volume bitdepth?
      uint16_t fadeLength;
      if (_parentSeq->version == AKAOSNES_V1) {
        fadeLength = GetShort(curOffset);
        curOffset += 2;
      }
      else {
        fadeLength = GetByte(curOffset++);
      }
      uint8_t _vol = GetByte(curOffset++);

      if (fadeLength != 0) {
        desc << L"Fade Length: " << fadeLength << L"  Volume: " << _vol;
        AddGenericEvent(beginOffset,
                        curOffset - beginOffset,
                        L"Volume Fade",
                        desc.str(),
                        CLR_VOLUME,
                        ICON_CONTROL);
      }
      else {
        AddVol(beginOffset, curOffset - beginOffset, _vol >> 1);
      }
      break;
    }

    case EVENT_PAN: {
      uint8_t pan = GetByte(curOffset++) << (_parentSeq->PAN_8BIT ? 0 : 1);

      uint8_t midiPan = ConvertLinearPercentPanValToStdMidiVal(pan / 255.0);

      // TODO: apply volume scale
      AddPan(beginOffset, curOffset - beginOffset, midiPan);
      break;
    }

    case EVENT_PAN_FADE: {
      uint16_t fadeLength;
      if (_parentSeq->version == AKAOSNES_V1) {
        fadeLength = GetShort(curOffset);
        curOffset += 2;
      }
      else {
        fadeLength = GetByte(curOffset++);
      }
      uint8_t pan = GetByte(curOffset++) << (_parentSeq->PAN_8BIT ? 0 : 1);

      uint8_t midiPan = ConvertLinearPercentPanValToStdMidiVal(pan / 255.0);

      // TODO: apply volume scale
      if (fadeLength != 0) {
        desc << L"Fade Length: " << fadeLength << L"  Pan: " << pan;
        AddGenericEvent(beginOffset, curOffset - beginOffset, L"Pan Fade", desc.str(), CLR_PAN, ICON_CONTROL);
      }
      else {
        AddPan(beginOffset, curOffset - beginOffset, midiPan);
      }
      break;
    }

    case EVENT_PITCH_SLIDE_ON: {
      int8_t pitchSlideSemitones;
      uint8_t pitchSlideDelay;
      uint8_t pitchSlideLength;

      if (_parentSeq->version == AKAOSNES_V1) {
        pitchSlideDelay = GetByte(curOffset++);
        pitchSlideLength = GetByte(curOffset++);
        pitchSlideSemitones = GetByte(curOffset++);
        desc << L"Delay: " << pitchSlideDelay << L"  Length: " << pitchSlideLength << L"  Key: "
            << pitchSlideSemitones << L" semitones";
      }
      else { // AKAOSNES_V2
        pitchSlideSemitones = GetByte(curOffset++);
        pitchSlideDelay = GetByte(curOffset++);
        pitchSlideLength = GetByte(curOffset++);
        desc << L"Key: " << pitchSlideSemitones << L" semitones  Delay: " << pitchSlideDelay
            << L"  Length: " << pitchSlideLength;
      }

      AddGenericEvent(beginOffset,
                      curOffset - beginOffset,
                      L"Pitch Slide On",
                      desc.str(),
                      CLR_PITCHBEND,
                      ICON_CONTROL);
      break;
    }

    case EVENT_PITCH_SLIDE_OFF: {
      AddGenericEvent(beginOffset,
                      curOffset - beginOffset,
                      L"Pitch Slide Off",
                      desc.str(),
                      CLR_PITCHBEND,
                      ICON_CONTROL);
      break;
    }

    case EVENT_PITCH_SLIDE: {
      uint8_t pitchSlideLength = GetByte(curOffset++);
      int8_t pitchSlideSemitones = GetByte(curOffset++);
      desc << L"Length: " << pitchSlideLength << L"  Key: " << pitchSlideSemitones << L" semitones";
      AddGenericEvent(beginOffset,
                      curOffset - beginOffset,
                      L"Pitch Slide",
                      desc.str(),
                      CLR_PITCHBEND,
                      ICON_CONTROL);
      break;
    }

    case EVENT_VIBRATO_ON: {
      uint8_t lfoDelay = GetByte(curOffset++);
      uint8_t lfoRate = GetByte(curOffset++);
      uint8_t lfoDepth = GetByte(curOffset++);
      desc << L"Delay: " << lfoDelay << L"  Rate: " << lfoRate << L"  Depth: " << lfoDepth;
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
      uint8_t lfoDelay = GetByte(curOffset++);
      uint8_t lfoRate = GetByte(curOffset++);
      uint8_t lfoDepth = GetByte(curOffset++);
      desc << L"Delay: " << lfoDelay << L"  Rate: " << lfoRate << L"  Depth: " << lfoDepth;
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

    case EVENT_PAN_LFO_ON_WITH_DELAY: {
      uint8_t lfoDelay = GetByte(curOffset++);
      uint8_t lfoRate = GetByte(curOffset++);
      uint8_t lfoDepth = GetByte(curOffset++);
      desc << L"Delay: " << lfoDelay << L"  Rate: " << lfoRate << L"  Depth: " << lfoDepth;
      AddGenericEvent(beginOffset,
                      curOffset - beginOffset,
                      L"Pan LFO",
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

    case EVENT_PITCHMOD_ON: {
      AddGenericEvent(beginOffset,
                      curOffset - beginOffset,
                      L"Pitch Modulation On",
                      desc.str(),
                      CLR_CHANGESTATE,
                      ICON_CONTROL);
      break;
    }

    case EVENT_PITCHMOD_OFF: {
      AddGenericEvent(beginOffset,
                      curOffset - beginOffset,
                      L"Pitch Modulation Off",
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

    case EVENT_OCTAVE: {
      uint8_t newOctave = GetByte(curOffset++);
      AddSetOctave(beginOffset, curOffset - beginOffset, newOctave);
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

    case EVENT_TRANSPOSE_ABS: {
      int8_t newTranspose = GetByte(curOffset++);
      AddTranspose(beginOffset, curOffset - beginOffset, newTranspose);
      break;
    }

    case EVENT_TRANSPOSE_REL: {
      int8_t newTranspose = GetByte(curOffset++);
      AddTranspose(beginOffset, curOffset - beginOffset, transpose + newTranspose, L"Transpose (Relative)");
      break;
    }

    case EVENT_TUNING: {
      uint8_t newTuning = GetByte(curOffset++);

      double pitchScale;
      if (newTuning <= 0x7f) {
        pitchScale = 1.0 + (newTuning / 256.0);
      }
      else {
        pitchScale = newTuning / 256.0;
      }

      double semitones = (log(pitchScale) / log(2.0)) * 12.0;
      AddFineTuning(beginOffset, curOffset - beginOffset, semitones * 100.0);
      break;
    }

    case EVENT_PROGCHANGE: {
      uint8_t newProg = GetByte(curOffset++);
      AddProgramChange(beginOffset, curOffset - beginOffset, newProg);
      nonPercussionProgram = newProg;
      break;
    }

    case EVENT_VOLUME_ENVELOPE: {
      uint8_t envelopeIndex = GetByte(curOffset++);
      desc << L"Envelope: " << envelopeIndex;
      AddGenericEvent(beginOffset,
                      curOffset - beginOffset,
                      L"Volume Envelope",
                      desc.str(),
                      CLR_VOLUME,
                      ICON_CONTROL);
      break;
    }

    case EVENT_GAIN_RELEASE: {
      uint8_t gain = GetByte(curOffset++) & 0x1f;
      desc << L"GAIN: " << gain;
      AddGenericEvent(beginOffset,
                      curOffset - beginOffset,
                      L"Release Rate (GAIN)",
                      desc.str(),
                      CLR_ADSR,
                      ICON_CONTROL);
      break;
    }

    case EVENT_DURATION_RATE: {
      uint8_t rate = GetByte(curOffset++);
      if (rate == 0 || rate == 100) {
        rate = 100;
      }

      desc << L"Note Length: " << rate << " %";
      AddGenericEvent(beginOffset,
                      curOffset - beginOffset,
                      L"Duration Rate",
                      desc.str(),
                      CLR_DURNOTE,
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

    case EVENT_ADSR_DEFAULT: {
      AddGenericEvent(beginOffset,
                      curOffset - beginOffset,
                      L"Default ADSR",
                      desc.str(),
                      CLR_ADSR,
                      ICON_CONTROL);
      break;
    }

    case EVENT_LOOP_START: {
      uint8_t count = GetByte(curOffset++);
      if (count != 0) {
        count++;
      }

      desc << L"Loop Count: " << count;
      AddGenericEvent(beginOffset, curOffset - beginOffset, L"Loop Start", desc.str(), CLR_LOOP, ICON_STARTREP);

      loopStart[loopLevel] = curOffset;
      if (_parentSeq->version == AKAOSNES_V4) {
        loopIncCount[loopLevel] = 1;
      }
      else { // AKAOSNES_V1, AKAOSNES_V2, AKAOSNES_V3
        loopIncCount[loopLevel] = 0;
      }
      loopDecCount[loopLevel] = count;
      loopLevel = (loopLevel + 1) % AKAOSNES_LOOP_LEVEL_MAX;
      break;
    }

    case EVENT_LOOP_END: {
      uint8_t prevLoopLevel = (loopLevel != 0 ? loopLevel : AKAOSNES_LOOP_LEVEL_MAX) - 1;
      if (_parentSeq->version == AKAOSNES_V4) {
        loopIncCount[prevLoopLevel]++;
      }

      if (loopDecCount[prevLoopLevel] == 0) {
        // infinite loop
        bContinue = AddLoopForever(beginOffset, curOffset - beginOffset);
        curOffset = loopStart[prevLoopLevel];
      }
      else {
        AddGenericEvent(beginOffset, curOffset - beginOffset, L"Loop End", desc.str(), CLR_LOOP, ICON_ENDREP);

        if (loopDecCount[prevLoopLevel] - 1 == 0) {
          // repeat end
          loopLevel = prevLoopLevel;
        }
        else {
          // repeat again
          loopDecCount[prevLoopLevel]--;
          curOffset = loopStart[prevLoopLevel];
        }
      }

      break;
    }

    case EVENT_LOOP_RESTART: {
      uint8_t prevLoopLevel = (loopLevel != 0 ? loopLevel : AKAOSNES_LOOP_LEVEL_MAX) - 1;

      // reset loop count, but keep the nest level
      loopIncCount[prevLoopLevel] = 0;

      break;
    }

    case EVENT_SLUR_ON: {
      AddGenericEvent(beginOffset,
                      curOffset - beginOffset,
                      L"Slur On (No Key Off/On)",
                      desc.str(),
                      CLR_CHANGESTATE,
                      ICON_CONTROL);
      slur = true;
      break;
    }

    case EVENT_SLUR_OFF: {
      AddGenericEvent(beginOffset,
                      curOffset - beginOffset,
                      L"Slur Off",
                      desc.str(),
                      CLR_CHANGESTATE,
                      ICON_CONTROL);
      slur = false;
      break;
    }

    case EVENT_LEGATO_ON: {
      AddGenericEvent(beginOffset,
                      curOffset - beginOffset,
                      L"Legato On (No Key Off)",
                      desc.str(),
                      CLR_CHANGESTATE,
                      ICON_CONTROL);
      legato = true;
      break;
    }

    case EVENT_LEGATO_OFF: {
      AddGenericEvent(beginOffset,
                      curOffset - beginOffset,
                      L"Legato Off",
                      desc.str(),
                      CLR_CHANGESTATE,
                      ICON_CONTROL);
      legato = false;
      break;
    }

    case EVENT_ONETIME_DURATION: {
      uint8_t dur = GetByte(curOffset++);
      onetimeDuration = dur;
      desc << L"Duration: " << dur;
      AddGenericEvent(beginOffset, curOffset - beginOffset, L"Duration (One-Time)", desc.str(), CLR_DURNOTE);
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

    case EVENT_PLAY_SFX: {
      // TODO: EVENT_PLAY_SFX
      uint8_t arg1 = GetByte(curOffset++);
      desc << L"Arg1: " << arg1;
      AddUnknown(beginOffset, curOffset - beginOffset, L"Play SFX", desc.str());
      break;
    }

    case EVENT_END: {
      AddEndOfTrack(beginOffset, curOffset - beginOffset);
      bContinue = false;
      break;
    }

    case EVENT_TEMPO: {
      uint8_t newTempo = GetByte(curOffset++);
      if (_parentSeq->minorVersion == AKAOSNES_V4_FM ||
          _parentSeq->minorVersion == AKAOSNES_V4_CT) {
        newTempo += (newTempo * 0x14) >> 8;
      }
      AddTempoBPM(beginOffset, curOffset - beginOffset, _parentSeq->GetTempoInBPM(newTempo));
      break;
    }

    case EVENT_TEMPO_FADE: {
      uint16_t fadeLength;
      if (_parentSeq->version == AKAOSNES_V1) {
        fadeLength = GetShort(curOffset);
        curOffset += 2;
      }
      else {
        fadeLength = GetByte(curOffset++);
      }

      uint8_t newTempo = GetByte(curOffset++);
      if (_parentSeq->minorVersion == AKAOSNES_V4_FM ||
          _parentSeq->minorVersion == AKAOSNES_V4_CT) {
        newTempo += (newTempo * 0x14) >> 8;
      }

      if (fadeLength != 0) {
        desc << L"Fade Length: " << fadeLength << L"  BPM: " << _parentSeq->GetTempoInBPM(newTempo);
        AddGenericEvent(beginOffset, curOffset - beginOffset, L"Tempo Fade", desc.str(), CLR_TEMPO, ICON_TEMPO);
      }
      else {
        AddTempoBPM(beginOffset, curOffset - beginOffset, _parentSeq->GetTempoInBPM(newTempo));
      }
      break;
    }

    case EVENT_ECHO_VOLUME: {
      uint8_t _vol = GetByte(curOffset++);
      desc << L"Volume: " << _vol;
      AddGenericEvent(beginOffset,
                      curOffset - beginOffset,
                      L"Echo Volume",
                      desc.str(),
                      CLR_REVERB,
                      ICON_CONTROL);
      break;
    }

    case EVENT_ECHO_VOLUME_FADE: {
      uint16_t fadeLength = GetByte(curOffset++);
      int8_t _vol = GetByte(curOffset++);

      desc << L"Fade Length: " << fadeLength << L"  Volume: " << _vol;
      AddGenericEvent(beginOffset,
                      curOffset - beginOffset,
                      L"Echo Volume Fade",
                      desc.str(),
                      CLR_REVERB,
                      ICON_CONTROL);
      break;
    }

    case EVENT_ECHO_FEEDBACK_FIR: {
      int8_t feedback = GetByte(curOffset++);
      uint8_t filterIndex = GetByte(curOffset++);
      desc << L"Feedback: " << feedback << L"  FIR: " << filterIndex;
      AddGenericEvent(beginOffset,
                      curOffset - beginOffset,
                      L"Echo Feedback & FIR",
                      desc.str(),
                      CLR_REVERB,
                      ICON_CONTROL);
      break;
    }

    case EVENT_MASTER_VOLUME: {
      uint8_t newVol = GetByte(curOffset++);
      AddMasterVol(beginOffset, curOffset - beginOffset, newVol / 2);
      break;
    }

    case EVENT_LOOP_BREAK: {
      uint8_t count = GetByte(curOffset++);
      uint16_t dest = GetShortAddress(curOffset);
      curOffset += 2;

      desc << L"Count: " << count << L"  Destination: $" << std::hex << std::setfill(L'0') << std::setw(4)
          << std::uppercase << dest;
      AddGenericEvent(beginOffset,
                      curOffset - beginOffset,
                      L"Loop Break / Jump to Volta",
                      desc.str(),
                      CLR_LOOP,
                      ICON_ENDREP);

      uint8_t prevLoopLevel = (loopLevel != 0 ? loopLevel : AKAOSNES_LOOP_LEVEL_MAX) - 1;
      if (_parentSeq->version != AKAOSNES_V4) { // AKAOSNES_V1, AKAOSNES_V2, AKAOSNES_V3, and Romancing SaGa 2
        // (Romancing SaGa 2 also increments the counter here actually, unlike other V4 games. VGMTrans doesn't try to emulate it at this time.)
        loopIncCount[prevLoopLevel]++;
      }

      // Branch if the incremental loop count matches the specified count.
      if (count == loopIncCount[prevLoopLevel]) {
        // The last iteration will end the loop (i.e. decreases the loop level)
        //
        // Note that V2 and V3 does not do this!
        // If you use an opcode to exit the innermost loop in a nested loop, the loop-level mismatch will break the nested loop.
        // (I don't think the songs officially released by Square use this opcode in a nested loop. That was probably not recommended.)
        if (_parentSeq->version == AKAOSNES_V1) {
          if (loopDecCount[prevLoopLevel] != 0) {
            // Only Final Fantasy 4 decreases the remaining count of the most-inner repeat.
            loopDecCount[prevLoopLevel]--;
            if (loopDecCount[prevLoopLevel] == 0) {
              loopLevel = prevLoopLevel;
            }
          }
        } else if (_parentSeq->version != AKAOSNES_V2 && _parentSeq->version != AKAOSNES_V3) {
          if (loopDecCount[prevLoopLevel] - 1 == 0) {
            loopLevel = prevLoopLevel;
          }
        }

        curOffset = dest;
      }

      break;
    }

    case EVENT_GOTO: {
      uint16_t dest = GetShortAddress(curOffset);
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

    case EVENT_INC_CPU_SHARED_COUNTER: {
      AddUnknown(beginOffset, curOffset - beginOffset, L"Increment CPU-Shared Counter", desc.str());
      break;
    }

    case EVENT_ZERO_CPU_SHARED_COUNTER: {
      AddUnknown(beginOffset, curOffset - beginOffset, L"Zero CPU-Shared Counter", desc.str());
      break;
    }

    case EVENT_ECHO_FEEDBACK_FADE: {
      uint16_t fadeLength = GetByte(curOffset++);
      uint8_t feedback = GetByte(curOffset++);

      desc << L"Fade Length: " << fadeLength << L"  Feedback: " << feedback;
      AddGenericEvent(beginOffset,
                      curOffset - beginOffset,
                      L"Echo Feedback Fade",
                      desc.str(),
                      CLR_REVERB,
                      ICON_CONTROL);
      break;
    }

    case EVENT_ECHO_FIR_FADE: {
      uint16_t fadeLength = GetByte(curOffset++);
//      uint8_t filterIndex = GetByte(curOffset++);

      desc << L"Fade Length: " << fadeLength << L"  FIR: " << vol;
      AddGenericEvent(beginOffset,
                      curOffset - beginOffset,
                      L"Echo FIR Fade",
                      desc.str(),
                      CLR_REVERB,
                      ICON_CONTROL);
      break;
    }

    case EVENT_ECHO_FEEDBACK: {
      uint8_t feedback = GetByte(curOffset++);
      desc << L"Feedback: " << feedback;
      AddGenericEvent(beginOffset,
                      curOffset - beginOffset,
                      L"Echo Feedback",
                      desc.str(),
                      CLR_REVERB,
                      ICON_CONTROL);
      break;
    }

    case EVENT_ECHO_FIR: {
//      uint8_t filterIndex = GetByte(curOffset++);
      desc << L"FIR: " << vol;
      AddGenericEvent(beginOffset, curOffset - beginOffset, L"Echo FIR", desc.str(), CLR_REVERB, ICON_CONTROL);
      break;
    }

    case EVENT_CPU_CONTROLED_SET_VALUE: {
//      uint8_t value = GetByte(curOffset++);
      AddUnknown(beginOffset, curOffset - beginOffset, L"Set Value for CPU-Controled Jump", desc.str());
      break;
    }

    case EVENT_CPU_CONTROLED_JUMP: {
      uint8_t arg1 = GetByte(curOffset++) & 15;
      uint16_t dest = GetShortAddress(curOffset);
      curOffset += 2;
      desc << L"Arg1: " << arg1 << L"  Destination: $" << std::hex << std::setfill(L'0') << std::setw(4)
          << std::uppercase << dest;
      AddUnknown(beginOffset, curOffset - beginOffset, L"CPU-Controled Jump", desc.str());
      break;
    }

    case EVENT_PERC_ON: {
      AddGenericEvent(beginOffset,
                      curOffset - beginOffset,
                      L"Percussion On",
                      desc.str(),
                      CLR_CHANGESTATE,
                      ICON_CONTROL);
      percussion = true;
      AddProgramChangeNoItem(AkaoSnesInstrSet::DRUMKIT_PROGRAM, true);
      break;
    }

    case EVENT_PERC_OFF: {
      AddGenericEvent(beginOffset,
                      curOffset - beginOffset,
                      L"Percussion Off",
                      desc.str(),
                      CLR_CHANGESTATE,
                      ICON_CONTROL);
      percussion = false;
      AddProgramChangeNoItem(nonPercussionProgram, true);
      break;
    }

    case EVENT_IGNORE_MASTER_VOLUME_BROKEN: {
      // multiply (1 << channel) / 256.0 to channel volume, instead of applying master volume
      // apparently, it is caused by wrong destination address of conditional branch
      AddGenericEvent(beginOffset,
                      curOffset - beginOffset,
                      L"Ignore Master Volume (Broken)",
                      desc.str(),
                      CLR_VOLUME,
                      ICON_CONTROL);
      break;
    }

    case EVENT_IGNORE_MASTER_VOLUME: {
      AddGenericEvent(beginOffset,
                      curOffset - beginOffset,
                      L"Ignore Master Volume",
                      desc.str(),
                      CLR_VOLUME,
                      ICON_CONTROL);
      break;
    }

    case EVENT_IGNORE_MASTER_VOLUME_BY_PROGNUM: {
      ignoreMasterVolumeProgNum = GetByte(curOffset++);
      desc << L"Program Number: " << ignoreMasterVolumeProgNum;
      AddGenericEvent(beginOffset,
                      curOffset - beginOffset,
                      L"Ignore Master Volume By Program Number",
                      desc.str(),
                      CLR_VOLUME,
                      ICON_CONTROL);
      break;
    }

    case EVENT_VOLUME_ALT: {
      uint8_t _vol = GetByte(curOffset++) & 0x7f;
      AddExpression(beginOffset, curOffset - beginOffset, _vol);
      break;
    }

    default:
      desc << L"Event: 0x" << std::hex << std::setfill(L'0') << std::setw(2) << std::uppercase << statusByte;
      AddUnknown(beginOffset, curOffset - beginOffset, L"Unknown Event", desc.str());
      pRoot->AddLogItem(new LogItem((std::wstring(L"Unknown Event - ") + desc.str()).c_str(),
                                    LOG_LEVEL_ERR,
                                    L"AkaoSnesSeq"));
      bContinue = false;
      break;
  }

  //wostringstream ssTrace;
  //ssTrace << L"" << std::hex << std::setfill(L'0') << std::setw(8) << std::uppercase << beginOffset << L": " << std::setw(2) <<statusByte  << L" -> " << std::setw(8) << curOffset << std::endl;
  //OutputDebugString(ssTrace.str().c_str());

  return bContinue;
}

uint16_t AkaoSnesTrack::ROMAddressToAPUAddress(uint16_t romAddress) {
  auto *_parentSeq = dynamic_cast<AkaoSnesSeq *>(this->parentSeq);
  return _parentSeq->ROMAddressToAPUAddress(romAddress);
}

uint16_t AkaoSnesTrack::GetShortAddress(uint32_t offset) {
  return ROMAddressToAPUAddress(GetShort(offset));
}
