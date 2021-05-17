#include "pch.h"
#include "PandoraBoxSnesInstr.h"
#include "SNESDSP.h"

// **********************
// PandoraBoxSnesInstrSet
// **********************

PandoraBoxSnesInstrSet::PandoraBoxSnesInstrSet(RawFile *file,
                                               PandoraBoxSnesVersion ver,
                                               uint32_t _spcDirAddr,
                                               uint16_t _addrLocalInstrTable,
                                               uint16_t _addrGlobalInstrTable,
                                               uint8_t _globalInstrumentCount,
                                               const std::map<uint8_t, uint16_t> &_instrADSRHints,
                                               const std::wstring &_name) :
    VGMInstrSet(PandoraBoxSnesFormat::name, file, _addrLocalInstrTable, 0, _name), version(ver),
    spcDirAddr(_spcDirAddr),
    addrLocalInstrTable(_addrLocalInstrTable),
    addrGlobalInstrTable(_addrGlobalInstrTable),
    globalInstrumentCount(_globalInstrumentCount),
    instrADSRHints(_instrADSRHints) {
}

PandoraBoxSnesInstrSet::~PandoraBoxSnesInstrSet() = default;

bool PandoraBoxSnesInstrSet::GetHeaderInfo() {
  if (globalInstrumentCount == 0) {
    return false;
  }

  if (addrGlobalInstrTable + globalInstrumentCount > 0x10000) {
    return false;
  }

  // read global instrument table into vector
  for (uint8_t globalInstrNum = 0; globalInstrNum < globalInstrumentCount; globalInstrNum++) {
    globalInstrTable.push_back(GetByte(addrGlobalInstrTable + globalInstrNum));
  }

  return true;
}

bool PandoraBoxSnesInstrSet::GetInstrPointers() {
  usedSRCNs.clear();

  for (uint8_t instrNum = 0; instrNum <= 0x7f; instrNum++) {
    uint32_t addrLocalInstrItem = addrLocalInstrTable + instrNum;
    if (addrLocalInstrItem >= 0x10000) {
      break;
    }

    uint8_t globalInstrNum = GetByte(addrLocalInstrItem);

    // search instrument number and get SRCN
    // note: actual engine do backward search, but I do not care
    auto iterInstrItem = std::find(globalInstrTable.begin(), globalInstrTable.end(), globalInstrNum);
    if (iterInstrItem == globalInstrTable.end()) {
      // out of range, perhaps?
      // actual music engine will use SRCN 0 for such case
      break;
    }
    uint8_t srcn = std::distance(globalInstrTable.begin(), iterInstrItem);

    uint32_t addrDIRentry = spcDirAddr + (srcn * 4);
    if (!SNESSampColl::IsValidSampleDir(rawfile, addrDIRentry, true)) {
      break;
    }

    auto itrSRCN = find(usedSRCNs.begin(), usedSRCNs.end(), srcn);
    if (itrSRCN == usedSRCNs.end()) {
      usedSRCNs.push_back(srcn);
    }

    uint16_t adsr = 0x8fe0;
    if (instrADSRHints.count(srcn)) {
      adsr = instrADSRHints[srcn];
    }

    std::wostringstream instrName;
    instrName << L"Instrument " << srcn;
    PandoraBoxSnesInstr *newInstr =
        new PandoraBoxSnesInstr(this, version, addrLocalInstrItem, instrNum, srcn, spcDirAddr, adsr, instrName.str());
    aInstrs.push_back(newInstr);
  }

  if (aInstrs.empty()) {
    return false;
  }

  std::sort(usedSRCNs.begin(), usedSRCNs.end());
  SNESSampColl *newSampColl = new SNESSampColl(PandoraBoxSnesFormat::name, this->rawfile, spcDirAddr, usedSRCNs);
  if (!newSampColl->LoadVGMFile()) {
    delete newSampColl;
    return false;
  }

  return true;
}

// *******************
// PandoraBoxSnesInstr
// *******************

PandoraBoxSnesInstr::PandoraBoxSnesInstr(VGMInstrSet *instrSet,
                                         PandoraBoxSnesVersion ver,
                                         uint32_t offset,
                                         uint8_t theInstrNum,
                                         uint8_t _srcn,
                                         uint32_t _spcDirAddr,
                                         uint16_t _adsr,
                                         const std::wstring &_name) :
    VGMInstr(instrSet, offset, 1, 0, theInstrNum, _name), version(ver),
    spcDirAddr(_spcDirAddr),
    srcn(_srcn),
    adsr(_adsr) {
}

PandoraBoxSnesInstr::~PandoraBoxSnesInstr() = default;

bool PandoraBoxSnesInstr::LoadInstr() {
  uint32_t offDirEnt = spcDirAddr + (srcn * 4);
  if (offDirEnt + 4 > 0x10000) {
    return false;
  }

  uint16_t addrSampStart = GetShort(offDirEnt);

  PandoraBoxSnesRgn *rgn = new PandoraBoxSnesRgn(this, version, dwOffset, srcn, spcDirAddr, adsr);
  rgn->sampOffset = addrSampStart - spcDirAddr;
  aRgns.push_back(rgn);

  SetGuessedLength();
  return true;
}

// *****************
// PandoraBoxSnesRgn
// *****************

PandoraBoxSnesRgn::PandoraBoxSnesRgn(PandoraBoxSnesInstr *instr,
                                     PandoraBoxSnesVersion ver,
                                     uint32_t offset,
                                     uint8_t srcn,
                                     uint32_t spcDirAddr,
                                     uint16_t adsr) :
    VGMRgn(instr, offset, 1),
    version(ver) {
  uint8_t adsr1 = adsr >> 8;
  uint8_t adsr2 = adsr & 0xff;

  AddSimpleItem(dwOffset, 1, L"Global Instrument #");

  sampNum = srcn;
  unityKey = 45; // o3a = $1000
  fineTune = 0;
  SNESConvADSR<VGMRgn>(this, adsr1, adsr2, 0x7f);
}

PandoraBoxSnesRgn::~PandoraBoxSnesRgn() = default;

bool PandoraBoxSnesRgn::LoadRgn() {
  return true;
}
