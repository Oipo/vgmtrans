#include "pch.h"
#include "VGMSampColl.h"

#include <utility>
#include "VGMSamp.h"
#include "VGMInstrSet.h"
#include "Root.h"

using namespace std;

// ***********
// VGMSampColl
// ***********

DECLARE_MENU(VGMSampColl)

VGMSampColl::VGMSampColl(const string &_format, RawFile *_rawfile, uint32_t offset, uint32_t length, wstring theName)
    : VGMFile(FILETYPE_SAMPCOLL, _format, _rawfile, offset, length, std::move(theName)),
      bLoadOnInstrSetMatch(false),
      bLoaded(false),
      sampDataOffset(0),
      parInstrSet(nullptr) {
  AddContainer<VGMSamp>(samples);
}

VGMSampColl::VGMSampColl(const string &_format, RawFile *_rawfile, VGMInstrSet *instrset,
                         uint32_t offset, uint32_t length, wstring theName)
    : VGMFile(FILETYPE_SAMPCOLL, _format, _rawfile, offset, length, std::move(theName)),
      bLoadOnInstrSetMatch(false),
      bLoaded(false),
      sampDataOffset(0),
      parInstrSet(instrset) {
  AddContainer<VGMSamp>(samples);
}

VGMSampColl::~VGMSampColl() {
  DeleteVect<VGMSamp>(samples);
}


bool VGMSampColl::Load() {
  if (bLoaded)
    return true;
  if (!GetHeaderInfo())
    return false;
  if (!GetSampleInfo())
    return false;

  if (samples.empty())
    return false;

  if (unLength == 0) {
    for (auto samp : samples) {
      // Some formats can have negative sample offset
      // For example, Konami's SNES format and Hudson's SNES format
      // TODO: Fix negative sample offset without breaking instrument
      //assert(dwOffset <= samp->dwOffset);

      //if (dwOffset > samp->dwOffset)
      //{
      //	unLength += samp->dwOffset - dwOffset;
      //	dwOffset = samp->dwOffset;
      //}

      if (dwOffset + unLength < samp->dwOffset + samp->unLength) {
        unLength = (samp->dwOffset + samp->unLength) - dwOffset;
      }
    }
  }

  UseRawFileData();
  if (!parInstrSet)
    pRoot->AddVGMFile(this);
  bLoaded = true;
  return true;
}


bool VGMSampColl::GetHeaderInfo() {
  return true;
}

bool VGMSampColl::GetSampleInfo() {
  return true;
}

VGMSamp *VGMSampColl::AddSamp(uint32_t offset, uint32_t length, uint32_t dataOffset, uint32_t dataLength,
                              uint8_t nChannels, uint16_t bps, uint32_t theRate, wstring _name) {
  VGMSamp *newSamp = new VGMSamp(this, offset, length, dataOffset, dataLength, nChannels,
                                 bps, theRate, std::move(_name));
  samples.push_back(newSamp);
  return newSamp;
}

bool VGMSampColl::OnSaveAllAsWav() {
  wstring dirpath = pRoot->UI_GetSaveDirPath();
  if (dirpath.length() != 0) {
    for (auto & sample : samples) {
      wstring filepath = dirpath + L"\\" + ConvertToSafeFileName(sample->sampName) + L".wav";
      sample->SaveAsWav(filepath);
    }
    return true;
  }
  return false;
}
