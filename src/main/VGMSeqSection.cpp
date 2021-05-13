#include "pch.h"
#include "common.h"
#include "VGMMultiSectionSeq.h"
#include "SeqEvent.h"

VGMSeqSection::VGMSeqSection(VGMMultiSectionSeq *parentFile,
                             uint32_t theOffset,
                             uint32_t theLength,
                             const std::wstring theName,
                             uint8_t _color)
    : VGMContainerItem(parentFile, theOffset, theLength, theName, _color),
      parentSeq(parentFile) {
  AddContainer<SeqTrack>(aTracks);
}

VGMSeqSection::~VGMSeqSection() {
  DeleteVect<SeqTrack>(aTracks);
}

bool VGMSeqSection::Load() {
  ReadMode readMode = parentSeq->readMode;

  if (readMode == READMODE_ADD_TO_UI) {
    if (!GetTrackPointers()) {
      return false;
    }
  }

  return true;
}

bool VGMSeqSection::GetTrackPointers() {
  return true;
}

bool VGMSeqSection::PostLoad() {
  if (parentSeq->readMode == READMODE_ADD_TO_UI) {
    for (auto & aTrack : aTracks) {
      std::sort(aTrack->aEvents.begin(), aTrack->aEvents.end(), ItemPtrOffsetCmp());
    }
  }

  return true;
}
