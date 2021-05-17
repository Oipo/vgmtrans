#include "pch.h"
#include "SoftCreatSnesSeq.h"

#include <utility>

DECLARE_FORMAT(SoftCreatSnes);

//  ****************
//  SoftCreatSnesSeq
//  ****************
#define MAX_TRACKS  8
#define SEQ_PPQN    48

SoftCreatSnesSeq::SoftCreatSnesSeq(RawFile *file,
                                   SoftCreatSnesVersion ver,
                                   uint32_t seqdataOffset,
                                   uint8_t _headerAlignSize,
                                   std::wstring newName)
    : VGMSeq(SoftCreatSnesFormat::name, file, seqdataOffset, 0, std::move(newName)), version(ver),
      headerAlignSize(_headerAlignSize) {
  bLoadTickByTick = true;
  bAllowDiscontinuousTrackData = true;
  bUseLinearAmplitudeScale = true;

  UseReverb();
  AlwaysWriteInitialReverb(0);

  LoadEventMap();
}

SoftCreatSnesSeq::~SoftCreatSnesSeq() = default;

void SoftCreatSnesSeq::ResetVars() {
  VGMSeq::ResetVars();
}

bool SoftCreatSnesSeq::GetHeaderInfo() {
  SetPPQN(SEQ_PPQN);

  VGMHeader *header = AddHeader(dwOffset, headerAlignSize * MAX_TRACKS);
  if (dwOffset + headerAlignSize * MAX_TRACKS > 0x10000) {
    return false;
  }

  for (uint8_t trackIndex = 0; trackIndex < MAX_TRACKS; trackIndex++) {
    uint32_t addrTrackLowPtr = dwOffset + (trackIndex * 2 * headerAlignSize);
    uint32_t addrTrackHighPtr = addrTrackLowPtr + headerAlignSize;
    if (addrTrackLowPtr + 1 > 0x10000 || addrTrackHighPtr + 1 > 0x10000) {
      return false;
    }

    std::wstringstream trackName;
    trackName << L"Track Pointer " << (trackIndex + 1);
    header->AddSimpleItem(addrTrackLowPtr, 1, trackName.str() + L" (LSB)");
    header->AddSimpleItem(addrTrackHighPtr, 1, trackName.str() + L" (MSB)");

    uint16_t addrTrackStart = GetByte(addrTrackLowPtr) | (GetByte(addrTrackHighPtr) << 8);
    if (addrTrackStart != 0xffff) {
      SoftCreatSnesTrack *track = new SoftCreatSnesTrack(this, addrTrackStart);
      aTracks.push_back(track);
    }
  }

  return true;
}

bool SoftCreatSnesSeq::GetTrackPointers() {
  return true;
}

void SoftCreatSnesSeq::LoadEventMap() const {
  if (version == SOFTCREATSNES_NONE) {
    return;
  }

  // TODO: SoftCreatSnesSeq::LoadEventMap
}


//  ******************
//  SoftCreatSnesTrack
//  ******************

SoftCreatSnesTrack::SoftCreatSnesTrack(SoftCreatSnesSeq *parentFile, long offset, long length)
    : SeqTrack(parentFile, offset, length) {
  ResetVars();
  bDetermineTrackLengthEventByEvent = true;
  bWriteGenericEventAsTextEvent = false;
}

void SoftCreatSnesTrack::ResetVars() {
  SeqTrack::ResetVars();
}

bool SoftCreatSnesTrack::ReadEvent() {
  SoftCreatSnesSeq *_parentSeq = dynamic_cast<SoftCreatSnesSeq *>(this->parentSeq);

  uint32_t beginOffset = curOffset;
  if (curOffset >= 0x10000) {
    return false;
  }

  uint8_t statusByte = GetByte(curOffset++);
  bool bContinue = true;

  std::wstringstream desc;

  auto eventType = static_cast<SoftCreatSnesSeqEventType>(0);
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

    default:
      desc << L"Event: 0x" << std::hex << std::setfill(L'0') << std::setw(2) << std::uppercase << statusByte;
      AddUnknown(beginOffset, curOffset - beginOffset, L"Unknown Event", desc.str());
      pRoot->AddLogItem(new LogItem((std::wstring(L"Unknown Event - ") + desc.str()).c_str(),
                                    LOG_LEVEL_ERR,
                                    L"SoftCreatSnesSeq"));
      bContinue = false;
      break;
  }

  //std::wostringstream ssTrace;
  //ssTrace << L"" << std::hex << std::setfill(L'0') << std::setw(8) << std::uppercase << beginOffset << L": " << std::setw(2) <<statusByte  << L" -> " << std::setw(8) << curOffset << std::endl;
  //OutputDebugString(ssTrace.str());

  return bContinue;
}
