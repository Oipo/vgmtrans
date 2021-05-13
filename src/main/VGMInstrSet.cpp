#include "pch.h"
#include "VGMInstrSet.h"

#include <utility>
#include "VGMSampColl.h"
#include "VGMRgn.h"
#include "VGMColl.h"
#include "Root.h"

DECLARE_MENU(VGMInstrSet)

using namespace std;

// ***********
// VGMInstrSet
// ***********

VGMInstrSet::VGMInstrSet(const string &_format,/*FmtID fmtID,*/
                         RawFile *file,
                         uint32_t offset,
                         uint32_t length,
                         wstring _name,
                         VGMSampColl *theSampColl)
    : VGMFile(FILETYPE_INSTRSET, /*fmtID,*/_format, file, offset, length, std::move(_name)), sampColl(theSampColl), allowEmptyInstrs(false) {
  AddContainer<VGMInstr>(aInstrs);
}

VGMInstrSet::~VGMInstrSet() {
  DeleteVect<VGMInstr>(aInstrs);
  delete sampColl;
}


VGMInstr *VGMInstrSet::AddInstr(uint32_t offset, uint32_t length, unsigned long bank,
                                unsigned long instrNum, const wstring &instrName) {
  wostringstream _name;
  if (instrName.empty())
    _name << L"Instrument " << aInstrs.size();
  else
    _name << instrName;

  VGMInstr *instr = new VGMInstr(this, offset, length, bank, instrNum, _name.str());
  aInstrs.push_back(instr);
  return instr;
}

bool VGMInstrSet::Load() {
  if (!GetHeaderInfo())
    return false;
  if (!GetInstrPointers())
    return false;
  if (!LoadInstrs())
    return false;

  if (!allowEmptyInstrs && aInstrs.empty())
    return false;

  if (unLength == 0) {
    SetGuessedLength();
  }

  if (sampColl != nullptr) {
    if (!sampColl->Load()) {
      pRoot->AddLogItem(new LogItem(L"Failed to load VGMSampColl.", LOG_LEVEL_ERR, L"VGMInstrSet"));
    }
  }

  LoadLocalData();
  UseLocalData();
  pRoot->AddVGMFile(this);
  return true;
}

bool VGMInstrSet::GetHeaderInfo() {
  return true;
}

bool VGMInstrSet::GetInstrPointers() {
  return true;
}


bool VGMInstrSet::LoadInstrs() {
  size_t nInstrs = aInstrs.size();
  for (size_t i = 0; i < nInstrs; i++) {
    if (!aInstrs[i]->LoadInstr())
      return false;
  }
  return true;
}


bool VGMInstrSet::OnSaveAsDLS() {
  wstring filepath = pRoot->UI_GetSaveFilePath(ConvertToSafeFileName(name), L"dls");
  if (filepath.length() != 0) {
    return SaveAsDLS(filepath.c_str());
  }
  return false;
}

bool VGMInstrSet::OnSaveAsSF2() {
  wstring filepath = pRoot->UI_GetSaveFilePath(ConvertToSafeFileName(name), L"sf2");
  if (filepath.length() != 0) {
    return SaveAsSF2(filepath);
  }
  return false;
}


bool VGMInstrSet::SaveAsDLS(const std::wstring &filepath) {
  DLSFile dlsfile;
  bool dlsCreationSucceeded = false;

  if (!assocColls.empty())
    dlsCreationSucceeded = assocColls.front()->CreateDLSFile(dlsfile);
  else
    return false;

  if (dlsCreationSucceeded) {
    return dlsfile.SaveDLSFile(filepath);
  }
  return false;
}

bool VGMInstrSet::SaveAsSF2(const std::wstring &filepath) {
  SF2File *sf2file = nullptr;

  if (!assocColls.empty())
    sf2file = assocColls.front()->CreateSF2File();
  else
    return false;
  if (sf2file != nullptr) {
    bool bResult = sf2file->SaveSF2File(filepath);
    delete sf2file;
    return bResult;
  }
  return false;
}


// ********
// VGMInstr
// ********

VGMInstr::VGMInstr(VGMInstrSet *instrSet, uint32_t offset, uint32_t length, uint32_t theBank,
                   uint32_t theInstrNum, const wstring &_name)
    : VGMContainerItem(instrSet, offset, length, _name),
      bank(theBank),
      instrNum(theInstrNum),
      parInstrSet(instrSet) {
  AddContainer<VGMRgn>(aRgns);
}

VGMInstr::~VGMInstr() {
  DeleteVect<VGMRgn>(aRgns);
}

void VGMInstr::SetBank(uint32_t bankNum) {
  bank = bankNum;
}

void VGMInstr::SetInstrNum(uint32_t theInstrNum) {
  instrNum = theInstrNum;
}

VGMRgn *VGMInstr::AddRgn(VGMRgn *rgn) {
  aRgns.push_back(rgn);
  return rgn;
}

VGMRgn *VGMInstr::AddRgn(uint32_t offset, uint32_t length, int sampNum, uint8_t keyLow, uint8_t keyHigh,
                         uint8_t velLow, uint8_t velHigh) {
  VGMRgn *newRgn = new VGMRgn(this, offset, length, keyLow, keyHigh, velLow, velHigh, sampNum);
  aRgns.push_back(newRgn);
  return newRgn;
}


bool VGMInstr::LoadInstr() {
  return true;
}

