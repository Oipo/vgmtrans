#include "pch.h"
#include "FalcomSnesInstr.h"
#include "SNESDSP.h"

// ******************
// FalcomSnesInstrSet
// ******************

FalcomSnesInstrSet::FalcomSnesInstrSet(RawFile *file,
                                       FalcomSnesVersion ver,
                                       uint32_t offset,
                                       uint32_t _addrSampToInstrTable,
                                       uint32_t _spcDirAddr,
                                       const std::map<uint8_t, uint16_t> &_instrADSRHints,
                                       const std::wstring &_name) :
    VGMInstrSet(FalcomSnesFormat::name, file, offset, 0, _name), version(ver),
    spcDirAddr(_spcDirAddr),
    addrSampToInstrTable(_addrSampToInstrTable),
    instrADSRHints(_instrADSRHints) {
}

FalcomSnesInstrSet::~FalcomSnesInstrSet() = default;

bool FalcomSnesInstrSet::GetHeaderInfo() {
  return true;
}

bool FalcomSnesInstrSet::GetInstrPointers() {
  const uint16_t kInstrItemSize = 5;

  usedSRCNs.clear();
  for (int instr = 0; instr < 255 / kInstrItemSize; instr++) {
    uint32_t addrInstrHeader = dwOffset + (kInstrItemSize * instr);
    if (addrInstrHeader + kInstrItemSize > 0x10000) {
      break;
    }

    // determine the sample number
    uint8_t srcn = 0;
    bool srcnDetermined = false;
    for (srcn = 0; srcn < 32; srcn++) {
      if (addrSampToInstrTable + srcn >= 0x10000) {
        break;
      }
      uint8_t value = GetByte(addrSampToInstrTable + srcn);
      if (value == instr) {
        srcnDetermined = true;
        break;
      }
    }
    if (!srcnDetermined) {
      continue;
    }

    uint32_t offDirEnt = spcDirAddr + (srcn * 4);
    if (offDirEnt + 4 > 0x10000) {
      break;
    }

    uint16_t addrSampStart = GetShort(offDirEnt);
    uint16_t addrLoopStart = GetShort(offDirEnt + 2);

    if (addrSampStart == 0x0000 && addrLoopStart == 0x0000) {
      continue;
    }

    if (addrSampStart < offDirEnt + 4) {
      continue;
    }

    auto itrSRCN = find(usedSRCNs.begin(), usedSRCNs.end(), srcn);
    if (itrSRCN == usedSRCNs.end()) {
      usedSRCNs.push_back(srcn);
    }

    std::wostringstream instrName;
    instrName << L"Instrument " << instr;
    FalcomSnesInstr *newInstr =
        new FalcomSnesInstr(this, version, addrInstrHeader, instr >> 7, instr & 0x7f, srcn, spcDirAddr, instrName.str());
    aInstrs.push_back(newInstr);
  }
  if (aInstrs.empty()) {
    return false;
  }

  std::sort(usedSRCNs.begin(), usedSRCNs.end());
  SNESSampColl *newSampColl = new SNESSampColl(FalcomSnesFormat::name, this->rawfile, spcDirAddr, usedSRCNs);
  if (!newSampColl->LoadVGMFile()) {
    delete newSampColl;
    return false;
  }

  return true;
}

// ***************
// FalcomSnesInstr
// ***************

FalcomSnesInstr::FalcomSnesInstr(VGMInstrSet *instrSet,
                                 FalcomSnesVersion ver,
                                 uint32_t offset,
                                 uint32_t theBank,
                                 uint32_t theInstrNum,
                                 uint8_t _srcn,
                                 uint32_t _spcDirAddr,
                                 const std::wstring &_name) :
    VGMInstr(instrSet, offset, 5, theBank, theInstrNum, _name), version(ver),
    srcn(_srcn),
    spcDirAddr(_spcDirAddr) {
}

FalcomSnesInstr::~FalcomSnesInstr() = default;

bool FalcomSnesInstr::LoadInstr() {
  uint32_t offDirEnt = spcDirAddr + (srcn * 4);
  if (offDirEnt + 4 > 0x10000) {
    return false;
  }

  uint16_t addrSampStart = GetShort(offDirEnt);

  FalcomSnesRgn *rgn = new FalcomSnesRgn(this, version, dwOffset, srcn);
  rgn->sampOffset = addrSampStart - spcDirAddr;
  aRgns.push_back(rgn);

  return true;
}

// *************
// FalcomSnesRgn
// *************

FalcomSnesRgn::FalcomSnesRgn(FalcomSnesInstr *instr,
                             FalcomSnesVersion ver,
                             uint32_t offset,
                             uint8_t srcn) :
    VGMRgn(instr, offset, 5), version(ver) {
  uint8_t adsr1 = GetByte(offset);
  uint8_t adsr2 = GetByte(offset + 1);
  int16_t pitch_scale = GetShortBE(offset + 3);

  // override ADSR
  //if (parInstrSet->instrADSRHints.count(instr->instrNum) != 0) {
  //  uint16_t adsr = parInstrSet->instrADSRHints[instr->instrNum];
  //  if (adsr != 0) {
  //    adsr1 = adsr & 0xff;
  //    adsr2 = (adsr >> 8) & 0xff;
  //  }
  //}

  const double pitch_fixer = 4286.0 / 4096.0;
  double fine_tuning;
  double coarse_tuning;
  fine_tuning = modf((log(pitch_scale * pitch_fixer / 256.0) / log(2.0)) * 12.0, &coarse_tuning);

  // normalize
  if (fine_tuning >= 0.5) {
    coarse_tuning += 1.0;
    fine_tuning -= 1.0;
  }
  else if (fine_tuning <= -0.5) {
    coarse_tuning -= 1.0;
    fine_tuning += 1.0;
  }

  sampNum = srcn;
  AddSimpleItem(offset, 1, L"ADSR1");
  AddSimpleItem(offset + 1, 1, L"ADSR2");
  AddUnityKey(96 - static_cast<int>(coarse_tuning), offset + 3, 1);
  AddFineTune(static_cast<int16_t> (fine_tuning * 100.0), offset + 4, 1);
  SNESConvADSR<VGMRgn>(this, adsr1, adsr2, 0);
}

FalcomSnesRgn::~FalcomSnesRgn() = default;

bool FalcomSnesRgn::LoadRgn() {
  return true;
}
