#include "pch.h"
#include "SonyPS2Seq.h"

DECLARE_FORMAT(SonyPS2);

using namespace std;

// ******
// BGMSeq
// ******

SonyPS2Seq::SonyPS2Seq(RawFile *file, uint32_t offset)
    : VGMSeqNoTrks(SonyPS2Format::name, file, offset),
      compOption(0),
      bSkipDeltaTime(false) {
  UseLinearAmplitudeScale();        // Onimusha: Kaede Theme track 2 for example of linear vol scale.
  UseReverb();
}

SonyPS2Seq::~SonyPS2Seq() = default;

bool SonyPS2Seq::GetHeaderInfo() {
  name() = L"Sony PS2 Seq";
  uint32_t _curOffset = offset();
  //read the version chunk
  GetBytes(_curOffset, 0x10, &versCk);
  VGMHeader *versCkHdr = VGMSeq::AddHeader(_curOffset, versCk.chunkSize, L"Version Chunk");
  versCkHdr->AddSimpleItem(_curOffset, 4, L"Creator");
  versCkHdr->AddSimpleItem(_curOffset + 4, 4, L"Type");
  _curOffset += versCk.chunkSize;

  //read the header chunk
  GetBytes(_curOffset, 0x20, &hdrCk);
  VGMHeader *hdrCkHdr = VGMSeq::AddHeader(_curOffset, hdrCk.chunkSize, L"Header Chunk");
  hdrCkHdr->AddSimpleItem(_curOffset, 4, L"Creator");
  hdrCkHdr->AddSimpleItem(_curOffset + 4, 4, L"Type");
  _curOffset += hdrCk.chunkSize;
  //Now we're at the Midi chunk, which starts with the sig "SCEIMidi" (in 32bit little endian)
  midiChunkSize = GetWord(_curOffset + 8);
  maxMidiNumber = GetWord(_curOffset + 12);
  //Get the first midi data block addr, which is provided relative to beginning of Midi chunk
  midiOffsetAddr = GetWord(_curOffset + 16) + _curOffset;
  _curOffset = midiOffsetAddr;
  //Now we're at the Midi Data Block
  uint32_t sequenceOffset = GetWord(_curOffset);        //read sequence offset
  SetEventsOffset(_curOffset + sequenceOffset);
  SetPPQN(GetShort(_curOffset + 4));                    //read ppqn value

  //if a compression mode is being applied
  if (sequenceOffset != 6) {
    compOption = GetShort(_curOffset + 6);            //read compression mode
  }

  nNumTracks = 16;
  channel = 0;
  SetCurTrack(channel);
  return true;
}


bool SonyPS2Seq::ReadEvent() {
  uint32_t beginOffset = curOffset;
  uint32_t _deltaTime;
  if (bSkipDeltaTime)
    _deltaTime = 0;
  else
    _deltaTime = ReadVarLen(curOffset);
  AddTime(_deltaTime);
  if (curOffset >= rawfile->size())
    return false;

  bSkipDeltaTime = false;

  uint8_t status_byte = GetByte(curOffset++);

  // Running Status
  if (status_byte <= 0x7F) {
    // some games were ripped to PSF with the EndTrack event missing, so
    // if we read a sequence of four 0 bytes, then just treat that as the end of the track
    if (status_byte == 0 && GetWord(curOffset) == 0) {
      return false;
    }
    status_byte = runningStatus;
    curOffset--;
  }
  else if (status_byte != 0xFF)
    runningStatus = status_byte;


  channel = status_byte & 0x0F;
  SetCurTrack(channel);

  switch (status_byte & 0xF0) {
    //note off event. Unlike SMF, there is no velocity data byte, just the note val.
    case 0x80 : {
      auto key = GetDataByte(curOffset++);
      AddNoteOff(beginOffset, curOffset - beginOffset, key);
    }
      break;

    //note on event (note off if velocity is zero)
    case 0x90 : {
      auto key = GetByte(curOffset++);
      auto vel = GetDataByte(curOffset++);
      if (vel > 0)  // if the velocity is > 0, it's a note on
        AddNoteOn(beginOffset, curOffset - beginOffset, key, vel);
      else  // otherwise it's a note off
        AddNoteOff(beginOffset, curOffset - beginOffset, key);
    }
      break;

    case 0xB0 : {
      uint8_t controlNum = GetByte(curOffset++);
      uint8_t value = GetDataByte(curOffset++);

      //control number
      switch (controlNum) {
        case 1 :
          AddModulation(beginOffset, curOffset - beginOffset, value);
          break;

        case 2 :
          AddBreath(beginOffset, curOffset - beginOffset, value);
          break;

        case 6 :
          //AddGenericEvent(beginOffset, curOffset-beginOffset, L"NRPN Data Entry", nullptr, BG_CLR_PINK);
          AddGenericEvent(beginOffset, curOffset - beginOffset, L"Loop start number", L"", CLR_LOOP);
          break;

        //volume
        case 7 :
          AddVol(beginOffset, curOffset - beginOffset, value);
          break;

        //pan
        case 10 :
          AddPan(beginOffset, curOffset - beginOffset, value);
          break;

        //expression
        case 11 :
          AddExpression(beginOffset, curOffset - beginOffset, value);
          break;

        //0 == endless loop
        case 38 :
          AddGenericEvent(beginOffset, curOffset - beginOffset, L"Loop count", L"", CLR_LOOP);
          break;

        //(0x63) nrpn msb
        case 99 :
          switch (value) {
            case 0 :
              AddGenericEvent(beginOffset, curOffset - beginOffset, L"Loop Start", L"", CLR_LOOP);
              break;

            case 1 :
              AddGenericEvent(beginOffset, curOffset - beginOffset, L"Loop End", L"", CLR_LOOP);
              break;
          }
          break;
      }
    }
      break;

    case 0xC0 : {
      uint8_t progNum = GetDataByte(curOffset++);
      AddProgramChange(beginOffset, curOffset - beginOffset, progNum);
    }
      break;

    case 0xE0 : {
      uint8_t hi = GetByte(curOffset++);
      uint8_t lo = GetDataByte(curOffset++);
      AddPitchBendMidiFormat(beginOffset, curOffset - beginOffset, hi, lo);
    }
      break;

    case 0xF0 : {
      if (status_byte == 0xFF) {
        switch (GetByte(curOffset++)) {
          //tempo. identical to SMF
          case 0x51 : {
            uint32_t microsPerQuarter = GetWordBE(curOffset) & 0x00FFFFFF;    //mask out the hi byte 0x03
            AddTempo(beginOffset, curOffset + 4 - beginOffset, microsPerQuarter);
            curOffset += 4;
            break;
          }

          case 0x2F :
            AddEndOfTrack(beginOffset, curOffset - beginOffset);
            return false;

          default :
            AddEndOfTrack(beginOffset, curOffset - beginOffset - 1);
            return false;
        }
      }
      break;
    }

    default:
      AddGenericEvent(beginOffset, curOffset - beginOffset, L"UNKNOWN", L"", CLR_UNRECOGNIZED);
      return false;
  }
  return true;
}

uint8_t SonyPS2Seq::GetDataByte(uint32_t offset) {
  uint8_t dataByte = GetByte(offset);
  if (dataByte & 0x80) {
    bSkipDeltaTime = true;
    dataByte &= 0x7F;
  }
  else
    bSkipDeltaTime = false;
  return dataByte;
}