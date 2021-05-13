#include "pch.h"
#include "Format.h"
#include "Matcher.h"

using namespace std;

FormatMap &Format::registry() {
  static FormatMap registry;
  return registry;
}

Format::Format(const string &formatName) {
  registry().insert(make_pair(formatName, this));
}

Format::~Format() {
    delete scanner;
    delete matcher;
}

Format *Format::GetFormatFromName(const string &name) {
  FormatMap::iterator findIt = registry().find(name);
  if (findIt == registry().end())
    return nullptr;
  return (*findIt).second;
}

bool Format::OnNewFile(VGMFile *file) {
  if (!matcher)
    return false;
  return matcher->OnNewFile(file);
  //switch (file->GetFileType())
  //{
  //case FILETYPE_SEQ:
  //	return OnNewSeq((VGMSeq*)file);
  //case FILETYPE_INSTRSET:
  //	return OnNewInstrSet((VGMInstrSet*)file);
  //case FILETYPE_SAMPCOLL:
  //	return OnNewSampColl((VGMSampColl*)file);
  //}
}

VGMColl *Format::NewCollection() {
  return new VGMColl();
}

bool Format::OnCloseFile(VGMFile *file) {
  if (!matcher)
    return false;
  return matcher->OnCloseFile(file);
}

bool Format::Init() {
  scanner = NewScanner();
  matcher = NewMatcher();
  return true;
}
