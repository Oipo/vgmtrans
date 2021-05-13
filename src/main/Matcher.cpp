#include "pch.h"
#include "Matcher.h"

using namespace std;

Matcher::Matcher(Format *format) {
  fmt = format;
}

Matcher::~Matcher() = default;

bool Matcher::OnNewFile(VGMFile *file) {
  switch (file->GetFileType()) {
    case FILETYPE_SEQ:
      return OnNewSeq(dynamic_cast<VGMSeq *>(file));
    case FILETYPE_INSTRSET:
      return OnNewInstrSet(dynamic_cast<VGMInstrSet *>(file));
    case FILETYPE_SAMPCOLL:
      return OnNewSampColl(dynamic_cast<VGMSampColl *>(file));
    case FILETYPE_UNDEFINED:
    case FILETYPE_MISC:
      break;
  }
  return false;
}

bool Matcher::OnCloseFile(VGMFile *file) {
  switch (file->GetFileType()) {
    case FILETYPE_SEQ:
      return OnCloseSeq(dynamic_cast<VGMSeq *>(file));
    case FILETYPE_INSTRSET:
      return OnCloseInstrSet(dynamic_cast<VGMInstrSet *>(file));
    case FILETYPE_SAMPCOLL:
      return OnCloseSampColl(dynamic_cast<VGMSampColl *>(file));
    case FILETYPE_UNDEFINED:
    case FILETYPE_MISC:
      break;
  }
  return false;
}

// ****************
// FilegroupMatcher
// ****************


FilegroupMatcher::FilegroupMatcher(Format *format)
    : Matcher(format) {
}

bool FilegroupMatcher::OnNewSeq(VGMSeq *seq) {
  seqs.push_back(seq);
  LookForMatch();
  return true;
}

bool FilegroupMatcher::OnNewInstrSet(VGMInstrSet *instrset) {
  instrsets.push_back(instrset);
  LookForMatch();
  return true;
}

bool FilegroupMatcher::OnNewSampColl(VGMSampColl *sampcoll) {

  sampcolls.push_back(sampcoll);
  LookForMatch();
  return true;
}

bool FilegroupMatcher::OnCloseSeq(VGMSeq *seq) {
  std::list<VGMSeq *>::iterator iterator = std::find(seqs.begin(), seqs.end(), seq);
  if (iterator != seqs.end()) {
    seqs.erase(iterator);
  }
  return true;
}

bool FilegroupMatcher::OnCloseInstrSet(VGMInstrSet *instrset) {
  std::list<VGMInstrSet *>::iterator iterator = std::find(instrsets.begin(), instrsets.end(), instrset);
  if (iterator != instrsets.end()) {
    instrsets.erase(iterator);
  }
  return true;
}

bool FilegroupMatcher::OnCloseSampColl(VGMSampColl *sampcoll) {
  std::list<VGMSampColl *>::iterator iterator = std::find(sampcolls.begin(), sampcolls.end(), sampcoll);
  if (iterator != sampcolls.end()) {
    sampcolls.erase(iterator);
  }
  return true;
}


void FilegroupMatcher::LookForMatch() {

  if (instrsets.size() == 1 && sampcolls.size() == 1) {
    if (seqs.size() >= 1) {
      for (list<VGMSeq *>::iterator iter = seqs.begin(); iter != seqs.end(); iter++) {
        VGMSeq *seq = *iter;
        VGMInstrSet *instrset = instrsets.front();
        VGMSampColl *sampcoll = sampcolls.front();
        VGMColl *coll = fmt->NewCollection();
        coll->SetName(seq->GetName());
        coll->UseSeq(seq);
        coll->AddInstrSet(instrset);
        coll->AddSampColl(sampcoll);
        if (!coll->Load()) {
          delete coll;
        }
      }
    }
    else {
      VGMInstrSet *instrset = instrsets.front();
      VGMSampColl *sampcoll = sampcolls.front();
      VGMColl *coll = fmt->NewCollection();
      coll->SetName(instrset->GetName());
      coll->UseSeq(nullptr);
      coll->AddInstrSet(instrset);
      coll->AddSampColl(sampcoll);
      if (!coll->Load()) {
        delete coll;
      }
    }
    seqs.clear();
    instrsets.clear();
    sampcolls.clear();
  }
}

template<class T>
T *FilegroupMatcher::GetLargestVGMFileInList(list<T *> theList) {
  uint32_t s = 0;
  T *curWinner = nullptr;
  for (typename list<T *>::iterator iter = theList.begin(); iter != theList.end(); iter++) {
    if ((*iter)->unLength > s) {
      s = (*iter)->unLength;
      curWinner = *iter;
    }
  }
  return curWinner;
}