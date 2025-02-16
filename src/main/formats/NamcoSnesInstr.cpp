#include "pch.h"
#include "NamcoSnesInstr.h"
#include "SNESDSP.h"

// *****************
// NamcoSnesInstrSet
// *****************

NamcoSnesInstrSet::NamcoSnesInstrSet(RawFile *file,
                                     NamcoSnesVersion ver,
                                     uint32_t _spcDirAddr,
                                     uint16_t _addrTuningTable,
                                     const std::wstring &_name) :
    VGMInstrSet(NamcoSnesFormat::name, file, _addrTuningTable, 0, _name), version(ver),
    spcDirAddr(_spcDirAddr),
    addrTuningTable(_addrTuningTable) {
}

NamcoSnesInstrSet::~NamcoSnesInstrSet() = default;

bool NamcoSnesInstrSet::GetHeaderInfo() {
  return true;
}

bool NamcoSnesInstrSet::GetInstrPointers() {
  uint8_t maxSampCount = 0x80;
  if (spcDirAddr < addrTuningTable) {
    uint16_t sampCountCandidate = (addrTuningTable - spcDirAddr) / 4;
    if (sampCountCandidate < maxSampCount) {
      maxSampCount = sampCountCandidate;
    }
  }

  usedSRCNs.clear();
  for (uint8_t srcn = 0; srcn < maxSampCount; srcn++) {
    uint32_t addrDIRentry = spcDirAddr + (srcn * 4);
    if (!SNESSampColl::IsValidSampleDir(rawfile, addrDIRentry, true)) {
      continue;
    }

    uint16_t addrSampStart = GetShort(addrDIRentry);
    if (addrSampStart < spcDirAddr) {
      continue;
    }

    uint32_t ofsTuningEntry;
    ofsTuningEntry = addrTuningTable + (srcn * 2);
    if (ofsTuningEntry + 2 > 0x10000) {
      break;
    }

    uint16_t sampleRate = GetShort(ofsTuningEntry);
    if (sampleRate == 0 || sampleRate == 0xffff) {
      continue;
    }

    usedSRCNs.push_back(srcn);

    std::wostringstream instrName;
    instrName << L"Instrument " << srcn;
    NamcoSnesInstr *newInstr = new NamcoSnesInstr(this, version, srcn, spcDirAddr, ofsTuningEntry, instrName.str());
    aInstrs.push_back(newInstr);
  }

  if (aInstrs.empty()) {
    return false;
  }

  std::sort(usedSRCNs.begin(), usedSRCNs.end());
  SNESSampColl *newSampColl = new SNESSampColl(NamcoSnesFormat::name, this->rawfile, spcDirAddr, usedSRCNs);
  if (!newSampColl->LoadVGMFile()) {
    delete newSampColl;
    return false;
  }

  return true;
}

// **************
// NamcoSnesInstr
// **************

NamcoSnesInstr::NamcoSnesInstr(VGMInstrSet *instrSet,
                               NamcoSnesVersion ver,
                               uint8_t srcn,
                               uint32_t _spcDirAddr,
                               uint16_t _addrTuningEntry,
                               const std::wstring &_name) :
    VGMInstr(instrSet, _addrTuningEntry, 0, 0, srcn, _name), version(ver),
    spcDirAddr(_spcDirAddr),
    addrTuningEntry(_addrTuningEntry) {
}

NamcoSnesInstr::~NamcoSnesInstr() = default;

bool NamcoSnesInstr::LoadInstr() {
  uint32_t offDirEnt = spcDirAddr + (instrNum * 4);
  if (offDirEnt + 4 > 0x10000) {
    return false;
  }

  uint16_t addrSampStart = GetShort(offDirEnt);

  NamcoSnesRgn *rgn = new NamcoSnesRgn(this, version, instrNum, spcDirAddr, addrTuningEntry);
  rgn->sampOffset = addrSampStart - spcDirAddr;
  aRgns.push_back(rgn);

  SetGuessedLength();
  return true;
}

// ************
// NamcoSnesRgn
// ************

NamcoSnesRgn::NamcoSnesRgn(NamcoSnesInstr *instr,
                           NamcoSnesVersion ver,
                           uint8_t srcn,
                           uint32_t spcDirAddr,
                           uint16_t addrTuningEntry) :
    VGMRgn(instr, addrTuningEntry, 0),
    version(ver) {
  int16_t pitch_scale = GetShortBE(addrTuningEntry);

  const double pitch_fixer = 4032.0 / 4096.0;
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

  AddSimpleItem(addrTuningEntry, 2, L"Sample Rate");
  unityKey = 71 - static_cast<int>(coarse_tuning);
  fineTune = static_cast<int16_t> (fine_tuning * 100.0);

  uint8_t adsr1 = 0x8f;
  uint8_t adsr2 = 0xe0;
  uint8_t gain = 0;
  SNESConvADSR<VGMRgn>(this, adsr1, adsr2, gain);

  SetGuessedLength();
}

NamcoSnesRgn::~NamcoSnesRgn() = default;

bool NamcoSnesRgn::LoadRgn() {
  return true;
}
