#include "pch.h"
#include "SuzukiSnesInstr.h"
#include "SNESDSP.h"

// ******************
// SuzukiSnesInstrSet
// ******************

SuzukiSnesInstrSet::SuzukiSnesInstrSet(RawFile *file,
                                       SuzukiSnesVersion ver,
                                       uint32_t _spcDirAddr,
                                       uint16_t _addrSRCNTable,
                                       uint16_t _addrVolumeTable,
                                       uint16_t _addrADSRTable,
                                       uint16_t _addrTuningTable,
                                       const std::wstring &_name) :
    VGMInstrSet(SuzukiSnesFormat::name, file, _addrSRCNTable, 0, _name), version(ver),
    spcDirAddr(_spcDirAddr),
    addrSRCNTable(_addrSRCNTable),
    addrVolumeTable(_addrVolumeTable),
    addrTuningTable(_addrTuningTable),
    addrADSRTable(_addrADSRTable) {
}

SuzukiSnesInstrSet::~SuzukiSnesInstrSet() = default;

bool SuzukiSnesInstrSet::GetHeaderInfo() {
  return true;
}

bool SuzukiSnesInstrSet::GetInstrPointers() {
  usedSRCNs.clear();
  for (uint8_t instrNum = 0; instrNum <= 0x7f; instrNum++) {
    uint32_t ofsSRCNEntry = addrSRCNTable + instrNum;
    if (ofsSRCNEntry + 1 > 0x10000) {
      continue;
    }
    uint8_t srcn = GetByte(ofsSRCNEntry);
    if (srcn >= 0x40) {
      continue;
    }

    uint32_t addrDIRentry = spcDirAddr + (srcn * 4);
    if (!SNESSampColl::IsValidSampleDir(rawfile, addrDIRentry, true)) {
      continue;
    }

    uint16_t addrSampStart = GetShort(addrDIRentry);
    if (addrSampStart < spcDirAddr) {
      continue;
    }

    uint32_t ofsVolumeEntry = addrVolumeTable + srcn * 2;
    if (ofsVolumeEntry + 1 > 0x10000) {
      break;
    }

    uint32_t ofsADSREntry = addrADSRTable + srcn * 2;
    if (ofsADSREntry + 2 > 0x10000) {
      break;
    }

    if (GetShort(ofsADSREntry) == 0x0000) {
      break;
    }

    uint32_t ofsTuningEntry = addrTuningTable + srcn * 2;
    if (ofsTuningEntry + 2 > 0x10000) {
      break;
    }

    if (GetShort(ofsTuningEntry) == 0xffff) {
      continue;
    }

    usedSRCNs.push_back(srcn);

    std::wostringstream instrName;
    instrName << L"Instrument " << srcn;
    SuzukiSnesInstr *newInstr = new SuzukiSnesInstr(this,
                                                    version,
                                                    instrNum,
                                                    spcDirAddr,
                                                    addrSRCNTable,
                                                    addrVolumeTable,
                                                    addrADSRTable,
                                                    addrTuningTable,
                                                    instrName.str());
    aInstrs.push_back(newInstr);
  }
  if (aInstrs.empty()) {
    return false;
  }

  std::sort(usedSRCNs.begin(), usedSRCNs.end());
  SNESSampColl *newSampColl = new SNESSampColl(SuzukiSnesFormat::name, this->rawfile, spcDirAddr, usedSRCNs);
  if (!newSampColl->LoadVGMFile()) {
    delete newSampColl;
    return false;
  }

  return true;
}

// ***************
// SuzukiSnesInstr
// ***************

SuzukiSnesInstr::SuzukiSnesInstr(VGMInstrSet *instrSet,
                                 SuzukiSnesVersion ver,
                                 uint8_t _instrNum,
                                 uint32_t _spcDirAddr,
                                 uint16_t _addrSRCNTable,
                                 uint16_t _addrVolumeTable,
                                 uint16_t _addrADSRTable,
                                 uint16_t _addrTuningTable,
                                 const std::wstring &_name) :
    VGMInstr(instrSet, _addrSRCNTable, 0, 0, _instrNum, _name), version(ver),
    spcDirAddr(_spcDirAddr),
    addrSRCNTable(_addrSRCNTable),
    addrVolumeTable(_addrVolumeTable),
    addrTuningTable(_addrTuningTable),
    addrADSRTable(_addrADSRTable) {
}

SuzukiSnesInstr::~SuzukiSnesInstr() = default;

bool SuzukiSnesInstr::LoadInstr() {
  uint32_t ofsADSREntry = addrSRCNTable + instrNum;
  if (ofsADSREntry + 1 > 0x10000) {
    return false;
  }
  uint8_t srcn = GetByte(ofsADSREntry);

  uint32_t offDirEnt = spcDirAddr + (srcn * 4);
  if (offDirEnt + 4 > 0x10000) {
    return false;
  }

  uint16_t addrSampStart = GetShort(offDirEnt);

  SuzukiSnesRgn *rgn = new SuzukiSnesRgn(this,
                                         version,
                                         instrNum,
                                         spcDirAddr,
                                         addrSRCNTable,
                                         addrVolumeTable,
                                         addrADSRTable,
                                         addrTuningTable);
  rgn->sampOffset = addrSampStart - spcDirAddr;
  aRgns.push_back(rgn);

  SetGuessedLength();
  return true;
}

// *************
// SuzukiSnesRgn
// *************

SuzukiSnesRgn::SuzukiSnesRgn(SuzukiSnesInstr *instr,
                             SuzukiSnesVersion ver,
                             uint8_t instrNum,
                             uint32_t spcDirAddr,
                             uint16_t addrSRCNTable,
                             uint16_t addrVolumeTable,
                             uint16_t addrADSRTable,
                             uint16_t addrTuningTable) :
    VGMRgn(instr, addrSRCNTable, 0),
    version(ver) {
  uint8_t srcn = GetByte(addrSRCNTable + instrNum);
  uint8_t vol = GetByte(addrVolumeTable + srcn * 2);
  uint8_t adsr1 = GetByte(addrADSRTable + srcn * 2);
  uint8_t adsr2 = GetByte(addrADSRTable + srcn * 2 + 1);
  uint8_t fine_tuning = GetByte(addrTuningTable + srcn * 2);
  int8_t coarse_tuning = GetByte(addrTuningTable + srcn * 2 + 1);

  AddSampNum(srcn, addrSRCNTable + instrNum, 1);
  AddSimpleItem(addrADSRTable + srcn * 2, 1, L"ADSR1");
  AddSimpleItem(addrADSRTable + srcn * 2 + 1, 1, L"ADSR2");
  AddFineTune(static_cast<int16_t>(fine_tuning / 256.0 * 100.0), addrTuningTable + srcn * 2, 1);
  AddUnityKey(69 - coarse_tuning, addrTuningTable + srcn * 2 + 1, 1);
  AddVolume(vol / 256.0, addrVolumeTable + srcn * 2, 1);
  SNESConvADSR<VGMRgn>(this, adsr1, adsr2, 0);

  SetGuessedLength();
}

SuzukiSnesRgn::~SuzukiSnesRgn() = default;

bool SuzukiSnesRgn::LoadRgn() {
  return true;
}
