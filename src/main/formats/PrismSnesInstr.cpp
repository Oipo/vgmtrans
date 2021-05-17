#include "pch.h"
#include "PrismSnesInstr.h"
#include "SNESDSP.h"

// *****************
// PrismSnesInstrSet
// *****************

PrismSnesInstrSet::PrismSnesInstrSet(RawFile *file,
                                     PrismSnesVersion ver,
                                     uint32_t _spcDirAddr,
                                     uint16_t _addrADSR1Table,
                                     uint16_t _addrADSR2Table,
                                     uint16_t _addrTuningTableHigh,
                                     uint16_t _addrTuningTableLow,
                                     const std::wstring &_name) :
    VGMInstrSet(PrismSnesFormat::name, file, _addrADSR1Table, 0, _name), version(ver),
    spcDirAddr(_spcDirAddr),
    addrADSR1Table(_addrADSR1Table),
    addrADSR2Table(_addrADSR2Table),
    addrTuningTableHigh(_addrTuningTableHigh),
    addrTuningTableLow(_addrTuningTableLow) {
}

PrismSnesInstrSet::~PrismSnesInstrSet() = default;

bool PrismSnesInstrSet::GetHeaderInfo() {
  return true;
}

bool PrismSnesInstrSet::GetInstrPointers() {
  usedSRCNs.clear();
  for (uint16_t srcn16 = 0; srcn16 < 0x100; srcn16++) {
    uint8_t srcn = srcn16;

    uint32_t addrDIRentry = spcDirAddr + (srcn * 4);
    if (!SNESSampColl::IsValidSampleDir(rawfile, addrDIRentry, true)) {
      continue;
    }

    uint16_t addrSampStart = GetShort(addrDIRentry);
    if (addrSampStart < spcDirAddr) {
      continue;
    }

    uint32_t ofsADSR1Entry;
    ofsADSR1Entry = addrADSR1Table + srcn;
    if (ofsADSR1Entry + 1 > 0x10000) {
      break;
    }

    uint32_t ofsADSR2Entry;
    ofsADSR2Entry = addrADSR2Table + srcn;
    if (ofsADSR2Entry + 1 > 0x10000) {
      break;
    }

    uint32_t ofsTuningEntryHigh;
    ofsTuningEntryHigh = addrTuningTableHigh + srcn;
    if (ofsTuningEntryHigh + 1 > 0x10000) {
      break;
    }

    uint32_t ofsTuningEntryLow;
    ofsTuningEntryLow = addrTuningTableLow + srcn;
    if (ofsTuningEntryLow + 1 > 0x10000) {
      break;
    }

    usedSRCNs.push_back(srcn);

    std::wostringstream instrName;
    instrName << L"Instrument " << srcn;
    PrismSnesInstr *newInstr = new PrismSnesInstr(this,
                                                  version,
                                                  srcn,
                                                  spcDirAddr,
                                                  ofsADSR1Entry,
                                                  ofsADSR2Entry,
                                                  ofsTuningEntryHigh,
                                                  ofsTuningEntryLow,
                                                  instrName.str());
    aInstrs.push_back(newInstr);
  }

  if (aInstrs.empty()) {
    return false;
  }

  std::sort(usedSRCNs.begin(), usedSRCNs.end());
  SNESSampColl *newSampColl = new SNESSampColl(PrismSnesFormat::name, this->rawfile, spcDirAddr, usedSRCNs);
  if (!newSampColl->LoadVGMFile()) {
    delete newSampColl;
    return false;
  }

  return true;
}

// **************
// PrismSnesInstr
// **************

PrismSnesInstr::PrismSnesInstr(VGMInstrSet *instrSet,
                               PrismSnesVersion ver,
                               uint8_t _srcn,
                               uint32_t _spcDirAddr,
                               uint16_t _addrADSR1Entry,
                               uint16_t _addrADSR2Entry,
                               uint16_t _addrTuningEntryHigh,
                               uint16_t _addrTuningEntryLow,
                               const std::wstring &_name) :
    VGMInstr(instrSet, _addrADSR1Entry, 0, _srcn >> 7, _srcn & 0x7f, _name), version(ver),
    srcn(_srcn),
    spcDirAddr(_spcDirAddr),
    addrADSR1Entry(_addrADSR1Entry),
    addrADSR2Entry(_addrADSR2Entry),
    addrTuningEntryHigh(_addrTuningEntryHigh),
    addrTuningEntryLow(_addrTuningEntryLow) {
}

PrismSnesInstr::~PrismSnesInstr() = default;

bool PrismSnesInstr::LoadInstr() {
  uint32_t offDirEnt = spcDirAddr + (srcn * 4);
  if (offDirEnt + 4 > 0x10000) {
    return false;
  }

  uint16_t addrSampStart = GetShort(offDirEnt);

  PrismSnesRgn *rgn = new PrismSnesRgn(this,
                                       version,
                                       srcn,
                                       spcDirAddr,
                                       addrADSR1Entry,
                                       addrADSR2Entry,
                                       addrTuningEntryHigh,
                                       addrTuningEntryLow);
  rgn->sampOffset = addrSampStart - spcDirAddr;
  aRgns.push_back(rgn);

  SetGuessedLength();
  return true;
}

// ************
// PrismSnesRgn
// ************

PrismSnesRgn::PrismSnesRgn(PrismSnesInstr *instr,
                           PrismSnesVersion ver,
                           uint8_t srcn,
                           uint32_t spcDirAddr,
                           uint16_t addrADSR1Entry,
                           uint16_t addrADSR2Entry,
                           uint16_t addrTuningEntryHigh,
                           uint16_t addrTuningEntryLow) :
    VGMRgn(instr, addrADSR1Entry, 0),
    version(ver) {
  int16_t tuning = GetByte(addrTuningEntryLow) | (GetByte(addrTuningEntryHigh) << 8);

  double fine_tuning;
  double coarse_tuning;
  fine_tuning = modf(tuning / 256.0, &coarse_tuning) * 100.0;

  AddSimpleItem(addrADSR1Entry, 1, L"ADSR1");
  AddSimpleItem(addrADSR2Entry, 1, L"ADSR2");
  AddUnityKey(93 - static_cast<int>(coarse_tuning), addrTuningEntryHigh, 1);
  AddFineTune(static_cast<int16_t>(fine_tuning), addrTuningEntryLow, 1);

  uint8_t adsr1 = GetByte(addrADSR1Entry);
  uint8_t adsr2 = GetByte(addrADSR2Entry);
  uint8_t gain = 0x9c;
  SNESConvADSR<VGMRgn>(this, adsr1, adsr2, gain);

  // put a random release time, it would be better than plain key off (actual music engine never do key off)
  release_time = LinAmpDecayTimeToLinDBDecayTime(0.002 * SDSP_COUNTER_RATES[0x14], 0x7ff);

  SetGuessedLength();
}

PrismSnesRgn::~PrismSnesRgn() = default;

bool PrismSnesRgn::LoadRgn() {
  return true;
}
