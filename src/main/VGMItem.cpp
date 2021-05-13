#include "pch.h"
#include "VGMItem.h"
#include "RawFile.h"
#include "VGMFile.h"
#include "Root.h"

using namespace std;

VGMItem::VGMItem()
    : color(0) {
}

VGMItem::VGMItem(VGMFile *thevgmfile, uint32_t theOffset, uint32_t theLength, const wstring theName, uint8_t theColor)
    : color(theColor),
      vgmfile(thevgmfile),
      name(theName),
      dwOffset(theOffset),
      unLength(theLength) {
}

VGMItem::~VGMItem() = default;

bool operator>(VGMItem &item1, VGMItem &item2) {
  return item1.dwOffset > item2.dwOffset;
}

bool operator<=(VGMItem &item1, VGMItem &item2) {
  return item1.dwOffset <= item2.dwOffset;
}

bool operator<(VGMItem &item1, VGMItem &item2) {
  return item1.dwOffset < item2.dwOffset;
}

bool operator>=(VGMItem &item1, VGMItem &item2) {
  return item1.dwOffset >= item2.dwOffset;
}


RawFile *VGMItem::GetRawFile() const {
  return vgmfile->rawfile;
}


bool VGMItem::IsItemAtOffset(uint32_t _offset, std::optional<bool> includeContainer, std::optional<bool> matchStartOffset) {
  if (GetItemFromOffset(_offset, includeContainer, matchStartOffset) != nullptr) {
    return true;
  }
  else {
    return false;
  }
}

VGMItem *VGMItem::GetItemFromOffset(uint32_t _offset, std::optional<bool> includeContainer, std::optional<bool> matchStartOffset) {
  if ((matchStartOffset.has_value() && matchStartOffset.value() ? _offset == dwOffset : _offset >= dwOffset) && (_offset < dwOffset + unLength)) {
    return this;
  }
  else {
    return nullptr;
  }
}

uint32_t VGMItem::GuessLength() {
  return unLength;
}

void VGMItem::SetGuessedLength() {
}

void VGMItem::AddToUI(VGMItem *parent, void *UI_specific) {
  pRoot->UI_AddItem(this, parent, name, UI_specific);
}

uint32_t VGMItem::GetBytes(uint32_t nIndex, uint32_t nCount, void *pBuffer) {
  return vgmfile->GetBytes(nIndex, nCount, pBuffer);
}

uint8_t VGMItem::GetByte(uint32_t _offset) {
  return vgmfile->GetByte(_offset);
}

uint16_t VGMItem::GetShort(uint32_t _offset) {
  return vgmfile->GetShort(_offset);
}

uint32_t VGMItem::GetWord(uint32_t _offset) {
  return GetRawFile()->GetWord(_offset);
}

//GetShort Big Endian
uint16_t VGMItem::GetShortBE(uint32_t _offset) {
  return GetRawFile()->GetShortBE(_offset);
}

//GetWord Big Endian
uint32_t VGMItem::GetWordBE(uint32_t _offset) {
  return GetRawFile()->GetWordBE(_offset);
}

bool VGMItem::IsValidOffset(uint32_t _offset) {
  return vgmfile->IsValidOffset(_offset);
}



//  ****************
//  VGMContainerItem
//  ****************

VGMContainerItem::VGMContainerItem()
    : VGMItem() {
  AddContainer(headers);
  AddContainer(localitems);
}


VGMContainerItem::VGMContainerItem(VGMFile *thevgmfile,
                                   uint32_t theOffset,
                                   uint32_t theLength,
                                   const wstring theName,
                                   uint8_t _color)
    : VGMItem(thevgmfile, theOffset, theLength, theName, _color) {
  AddContainer(headers);
  AddContainer(localitems);
}

VGMContainerItem::~VGMContainerItem() {
  DeleteVect(headers);
  DeleteVect(localitems);
}

VGMItem *VGMContainerItem::GetItemFromOffset(uint32_t _offset, std::optional<bool> includeContainer, std::optional<bool> matchStartOffset) {
  for (auto & container : containers) {
    for (auto item : *container) {
      if (item->unLength == 0 || (_offset >= item->dwOffset && _offset < item->dwOffset + item->unLength)) {
        VGMItem *foundItem = item->GetItemFromOffset(_offset, includeContainer, matchStartOffset);
        if (foundItem)
          return foundItem;
      }
    }
  }

  if (includeContainer.has_value() && includeContainer.value() && (matchStartOffset.has_value() && matchStartOffset.value() ? _offset == dwOffset : _offset >= dwOffset)
      && (_offset < dwOffset + unLength)) {
    return this;
  }
  else {
    return nullptr;
  }
}

// Guess length of a container from its descendants
uint32_t VGMContainerItem::GuessLength() {
  uint32_t guessedLength = 0;

  // Note: children items can sometimes overwrap each other
  for (auto & container : containers) {
    for (auto item : *container) {
      assert(dwOffset <= item->dwOffset);

      uint32_t itemLength = item->unLength;
      if (unLength == 0) {
        itemLength = item->GuessLength();
      }

      uint32_t expectedLength = item->dwOffset + itemLength - dwOffset;
      if (guessedLength < expectedLength) {
        guessedLength = expectedLength;
      }
    }
  }

  return guessedLength;
}

void VGMContainerItem::SetGuessedLength() {
  for (auto & container : containers) {
    for (auto item : *container) {
      item->SetGuessedLength();
    }
  }

  if (unLength == 0) {
    unLength = GuessLength();
  }
}

void VGMContainerItem::AddToUI(VGMItem *parent, void *UI_specific) {
  VGMItem::AddToUI(parent, UI_specific);
  for (auto & container : containers) {
    for (auto &item : *container)
      item->AddToUI(this, UI_specific);
  }
}

VGMHeader *VGMContainerItem::AddHeader(uint32_t _offset, uint32_t length, const std::wstring &_name) {
  VGMHeader *header = new VGMHeader(this, _offset, length, _name);
  headers.push_back(header);
  return header;
}

void VGMContainerItem::AddItem(VGMItem *item) {
  localitems.push_back(item);
}

void VGMContainerItem::AddSimpleItem(uint32_t _offset, uint32_t length, const std::wstring &_name) {
  localitems.push_back(new VGMItem(this->vgmfile, _offset, length, _name, CLR_HEADER));
}

void VGMContainerItem::AddUnknownItem(uint32_t _offset, uint32_t length) {
  localitems.push_back(new VGMItem(this->vgmfile, _offset, length, L"Unknown"));
}
