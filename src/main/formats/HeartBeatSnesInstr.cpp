#include "pch.h"
#include "HeartBeatSnesInstr.h"
#include "SNESDSP.h"

// *********************
// HeartBeatSnesInstrSet
// *********************

HeartBeatSnesInstrSet::HeartBeatSnesInstrSet(RawFile *file,
                                             HeartBeatSnesVersion ver,
                                             uint32_t offset,
                                             uint32_t length,
                                             uint16_t _addrSRCNTable,
                                             uint8_t _songIndex,
                                             uint32_t _spcDirAddr,
                                             const std::wstring &_name) :
    VGMInstrSet(HeartBeatSnesFormat::name, file, offset, length, _name), version(ver),
    addrSRCNTable(_addrSRCNTable),
    songIndex(_songIndex),
    spcDirAddr(_spcDirAddr) {
}

HeartBeatSnesInstrSet::~HeartBeatSnesInstrSet() = default;

bool HeartBeatSnesInstrSet::GetHeaderInfo() {
  return true;
}

bool HeartBeatSnesInstrSet::GetInstrPointers() {
  usedSRCNs.clear();

  uint8_t nNumInstrs = unLength / 6;
  if (nNumInstrs == 0) {
    return false;
  }

  for (uint8_t instrNum = 0; instrNum < nNumInstrs; instrNum++) {
    uint32_t addrInstrHeader = dwOffset + (instrNum * 6);
    if (addrInstrHeader + 6 > 0x10000) {
      break;
    }

    uint8_t sampleIndex = GetByte(addrInstrHeader) + (songIndex * 0x10);
    if (addrSRCNTable + sampleIndex + 1 > 0x10000) {
      break;
    }

    uint8_t srcn = GetByte(addrSRCNTable + sampleIndex);

    uint32_t offDirEnt = spcDirAddr + (srcn * 4);
    uint16_t addrSampStart = GetShort(offDirEnt);
    if (addrSampStart < offDirEnt + 4) {
      continue;
    }

    if (!SNESSampColl::IsValidSampleDir(rawfile, offDirEnt, true)) {
      // safety logic
      unLength = instrNum * 6;
      break;
    }

    auto itrSRCN = std::find(usedSRCNs.begin(), usedSRCNs.end(), srcn);
    if (itrSRCN == usedSRCNs.end()) {
      usedSRCNs.push_back(srcn);
    }

    std::wostringstream instrName;
    instrName << L"Instrument " << instrNum;
    HeartBeatSnesInstr *newInstr = new HeartBeatSnesInstr(this,
                                                          version,
                                                          addrInstrHeader,
                                                          instrNum >> 7,
                                                          instrNum & 0x7f,
                                                          addrSRCNTable,
                                                          songIndex,
                                                          spcDirAddr,
                                                          instrName.str());
    aInstrs.push_back(newInstr);
  }

  if (aInstrs.empty()) {
    return false;
  }

  std::sort(usedSRCNs.begin(), usedSRCNs.end());
  SNESSampColl *newSampColl = new SNESSampColl(HeartBeatSnesFormat::name, this->rawfile, spcDirAddr, usedSRCNs);
  if (!newSampColl->LoadVGMFile()) {
    delete newSampColl;
    return false;
  }

  return true;
}

// ******************
// HeartBeatSnesInstr
// ******************

HeartBeatSnesInstr::HeartBeatSnesInstr(VGMInstrSet *instrSet,
                                       HeartBeatSnesVersion ver,
                                       uint32_t offset,
                                       uint32_t theBank,
                                       uint32_t theInstrNum,
                                       uint16_t _addrSRCNTable,
                                       uint8_t _songIndex,
                                       uint32_t _spcDirAddr,
                                       const std::wstring &_name) :
    VGMInstr(instrSet, offset, 6, theBank, theInstrNum, _name), version(ver),
    addrSRCNTable(_addrSRCNTable),
    songIndex(_songIndex),
    spcDirAddr(_spcDirAddr) {
}

HeartBeatSnesInstr::~HeartBeatSnesInstr() = default;

bool HeartBeatSnesInstr::LoadInstr() {
  uint8_t sampleIndex = GetByte(dwOffset) + (songIndex * 0x10);
  if (addrSRCNTable + sampleIndex + 1 > 0x10000) {
    return false;
  }

  uint8_t srcn = GetByte(addrSRCNTable + sampleIndex);

  uint32_t offDirEnt = spcDirAddr + (srcn * 4);
  if (offDirEnt + 4 > 0x10000) {
    return false;
  }

  uint16_t addrSampStart = GetShort(offDirEnt);

  HeartBeatSnesRgn *rgn = new HeartBeatSnesRgn(this, version, dwOffset);
  rgn->sampOffset = addrSampStart - spcDirAddr;
  aRgns.push_back(rgn);

  return true;
}

// ****************
// HeartBeatSnesRgn
// ****************

HeartBeatSnesRgn::HeartBeatSnesRgn(HeartBeatSnesInstr *instr, HeartBeatSnesVersion ver, uint32_t offset) :
    VGMRgn(instr, offset, 6), version(ver) {
  uint8_t srcn = GetByte(offset);
  uint8_t adsr1 = GetByte(offset + 1);
  uint8_t adsr2 = GetByte(offset + 2);
  uint8_t gain = GetByte(offset + 3);
  int16_t pitch_scale = GetShortBE(offset + 4);

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

  AddSampNum(srcn, offset, 1);
  AddSimpleItem(offset + 1, 1, L"ADSR1");
  AddSimpleItem(offset + 2, 1, L"ADSR2");
  AddSimpleItem(offset + 3, 1, L"GAIN");
  AddUnityKey(72 - static_cast<int>(coarse_tuning), offset + 4, 1);
  AddFineTune(static_cast<int16_t>(fine_tuning * 100.0), offset + 5, 1);
  SNESConvADSR<VGMRgn>(this, adsr1, adsr2, gain);

  // use ADSR sustain for release rate
  // actual music engine sets sustain rate to release rate, it's useless,
  // so here I put a random value commonly used
  // TODO: obtain proper release rate from sequence
  uint8_t sr_release = 0x1c;
  ConvertSNESADSR(adsr1, (adsr2 & 0xe0) | sr_release, gain, 0x7ff, nullptr, nullptr, nullptr, &this->release_time, nullptr);
}

HeartBeatSnesRgn::~HeartBeatSnesRgn() = default;

bool HeartBeatSnesRgn::LoadRgn() {
  return true;
}
