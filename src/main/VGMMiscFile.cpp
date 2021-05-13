#include "pch.h"
#include "VGMMiscFile.h"

#include <utility>
#include "Root.h"

using namespace std;

// ***********
// VGMMiscFile
// ***********

VGMMiscFile::VGMMiscFile(const string &_format, RawFile *file, uint32_t _offset, uint32_t length, wstring _name)
    : VGMFile(FILETYPE_MISC, _format, file, _offset, length, std::move(_name)) {

}

bool VGMMiscFile::LoadMain() {
  return true;
}

bool VGMMiscFile::Load() {
  if (!LoadMain())
    return false;
  if (unLength == 0)
    return false;

  LoadLocalData();
  UseLocalData();
  pRoot->AddVGMFile(this);
  return true;
}
