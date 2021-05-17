#include "pch.h"
#include "MoriSnesInstr.h"

#include <utility>
#include "SNESDSP.h"

// ****************
// MoriSnesInstrSet
// ****************

MoriSnesInstrSet::MoriSnesInstrSet(RawFile *file,
                                   MoriSnesVersion ver,
                                   uint32_t _spcDirAddr,
                                   std::vector<uint16_t> _instrumentAddresses,
                                   std::map<uint16_t, MoriSnesInstrHintDir> _instrumentHints,
                                   const std::wstring &_name) :
    VGMInstrSet(MoriSnesFormat::name, file, 0, 0, _name), version(ver),
    spcDirAddr(_spcDirAddr),
    instrumentAddresses(std::move(_instrumentAddresses)),
    instrumentHints(std::move(_instrumentHints)) {
}

MoriSnesInstrSet::~MoriSnesInstrSet() = default;

bool MoriSnesInstrSet::GetHeaderInfo() {
  return true;
}

bool MoriSnesInstrSet::GetInstrPointers() {
  usedSRCNs.clear();

  if (instrumentAddresses.empty()) {
    return false;
  }

  // calculate whole instrument collection size
  uint16_t instrSetStartAddress = 0xffff;
  uint16_t instrSetEndAddress = 0;
  for (unsigned short instrAddress : instrumentAddresses) {
    uint16_t instrStartAddress = instrAddress;
    uint16_t instrEndAddress = instrAddress;
    if (!instrumentHints[instrAddress].percussion) {
      MoriSnesInstrHint *instrHint = &instrumentHints[instrAddress].instrHint;
      if (instrHint->startAddress < instrStartAddress) {
        instrStartAddress = instrHint->startAddress;
      }
      if (instrHint->startAddress + instrHint->size > instrEndAddress) {
        instrEndAddress = instrHint->startAddress + instrHint->size;
      }
    }
    else {
      for (auto & percHint : instrumentHints[instrAddress].percHints) {
        MoriSnesInstrHint *instrHint = &percHint;
        if (instrHint->startAddress < instrStartAddress) {
          instrStartAddress = instrHint->startAddress;
        }
        if (instrHint->startAddress + instrHint->size > instrEndAddress) {
          instrEndAddress = instrHint->startAddress + instrHint->size;
        }
      }
    }
    instrumentHints[instrAddress].startAddress = instrStartAddress;
    instrumentHints[instrAddress].size = instrEndAddress - instrStartAddress;

    if (instrStartAddress < instrSetStartAddress) {
      instrSetStartAddress = instrStartAddress;
    }
    if (instrEndAddress > instrSetEndAddress) {
      instrSetEndAddress = instrEndAddress;
    }
  }
  dwOffset = instrSetStartAddress;
  unLength = instrSetEndAddress - instrSetStartAddress;

  // load each instruments
  for (uint8_t instrNum = 0; instrNum < instrumentAddresses.size(); instrNum++) {
    uint16_t instrAddress = instrumentAddresses[instrNum];

    if (!instrumentHints[instrAddress].percussion) {
      MoriSnesInstrHint *instrHint = &instrumentHints[instrAddress].instrHint;

      uint16_t rgnAddress = instrHint->rgnAddress;
      if (rgnAddress == 0 || rgnAddress + 7 > 0x10000) {
        continue;
      }

      uint8_t srcn = GetByte(rgnAddress);
      auto itrSRCN = find(usedSRCNs.begin(), usedSRCNs.end(), srcn);
      if (itrSRCN == usedSRCNs.end()) {
        usedSRCNs.push_back(srcn);
      }
    }
    else {
      for (auto & percHint : instrumentHints[instrAddress].percHints) {
        MoriSnesInstrHint *instrHint = &percHint;

        uint16_t rgnAddress = instrHint->rgnAddress;
        if (rgnAddress == 0 || rgnAddress + 7 > 0x10000) {
          continue;
        }

        uint8_t srcn = GetByte(rgnAddress);
        auto itrSRCN = find(usedSRCNs.begin(), usedSRCNs.end(), srcn);
        if (itrSRCN == usedSRCNs.end()) {
          usedSRCNs.push_back(srcn);
        }
      }
    }

    std::wostringstream instrName;
    instrName << L"Instrument " << instrNum;
    MoriSnesInstr *newInstr =
        new MoriSnesInstr(this, version, instrNum, spcDirAddr, instrumentHints[instrAddress], instrName.str());
    aInstrs.push_back(newInstr);
  }

  if (aInstrs.empty()) {
    return false;
  }

  std::sort(usedSRCNs.begin(), usedSRCNs.end());
  SNESSampColl *newSampColl = new SNESSampColl(MoriSnesFormat::name, this->rawfile, spcDirAddr, usedSRCNs);
  if (!newSampColl->LoadVGMFile()) {
    delete newSampColl;
    return false;
  }

  return true;
}

// *************
// MoriSnesInstr
// *************

MoriSnesInstr::MoriSnesInstr(VGMInstrSet *instrSet,
                             MoriSnesVersion ver,
                             uint8_t _instrNum,
                             uint32_t _spcDirAddr,
                             const MoriSnesInstrHintDir &_instrHintDir,
                             const std::wstring &_name) :
    VGMInstr(instrSet, _instrHintDir.startAddress, _instrHintDir.size, 0, _instrNum, _name), version(ver),
    spcDirAddr(_spcDirAddr),
    instrHintDir(_instrHintDir) {
}

MoriSnesInstr::~MoriSnesInstr() = default;

bool MoriSnesInstr::LoadInstr() {
  AddSimpleItem(dwOffset, 1, L"Melody/Percussion");

  if (!instrHintDir.percussion) {
    MoriSnesInstrHint *instrHint = &instrHintDir.instrHint;
    uint8_t srcn = GetByte(instrHint->rgnAddress);

    uint32_t offDirEnt = spcDirAddr + (srcn * 4);
    if (offDirEnt + 4 > 0x10000) {
      return false;
    }

    AddSimpleItem(instrHint->seqAddress, instrHint->seqSize, L"Envelope Sequence");

    uint16_t addrSampStart = GetShort(offDirEnt);
    MoriSnesRgn *rgn = new MoriSnesRgn(this, version, spcDirAddr, *instrHint);
    rgn->sampOffset = addrSampStart - spcDirAddr;
    aRgns.push_back(rgn);
  }
  else {
    for (uint8_t percNoteKey = 0; percNoteKey < instrHintDir.percHints.size(); percNoteKey++) {
      MoriSnesInstrHint *instrHint = &instrHintDir.percHints[percNoteKey];
      uint8_t srcn = GetByte(instrHint->rgnAddress);

      uint32_t offDirEnt = spcDirAddr + (srcn * 4);
      if (offDirEnt + 4 > 0x10000) {
        return false;
      }

      std::wostringstream seqOffsetName;
      seqOffsetName << L"Sequence Offset " << percNoteKey;
      AddSimpleItem(dwOffset + 1 + (percNoteKey * 2), 2, seqOffsetName.str());

      std::wostringstream seqName;
      seqName << L"Envelope Sequence " << percNoteKey;
      AddSimpleItem(instrHint->seqAddress, instrHint->seqSize, seqName.str());

      uint16_t addrSampStart = GetShort(offDirEnt);
      MoriSnesRgn *rgn = new MoriSnesRgn(this, version, spcDirAddr, *instrHint, percNoteKey);
      rgn->sampOffset = addrSampStart - spcDirAddr;
      aRgns.push_back(rgn);
    }
  }

  return true;
}

// ***********
// MoriSnesRgn
// ***********

MoriSnesRgn::MoriSnesRgn(MoriSnesInstr *instr,
                         MoriSnesVersion ver,
                         uint32_t spcDirAddr,
                         const MoriSnesInstrHint &instrHint,
                         int8_t percNoteKey) :
    VGMRgn(instr, instrHint.rgnAddress, 7),
    version(ver) {
  uint16_t rgnAddress = instrHint.rgnAddress;
  uint16_t curOffset = rgnAddress;

  uint8_t srcn = GetByte(curOffset++);
  uint8_t adsr1 = GetByte(curOffset++);
  uint8_t adsr2 = GetByte(curOffset++);
  uint8_t gain = GetByte(curOffset++);
//  uint8_t keyOffDelay = GetByte(curOffset++);
  int8_t key = GetByte(curOffset++);
  uint8_t tuning = GetByte(curOffset++);

  double fine_tuning;
  double coarse_tuning;
  const double pitch_fixer = log(4096.0 / 4286.0) / log(2); // from pitch table ($10be vs $1000)
  fine_tuning = modf((key + (tuning / 256.0)) + pitch_fixer, &coarse_tuning);

  // normalize
  if (fine_tuning >= 0.5) {
    coarse_tuning += 1.0;
    fine_tuning -= 1.0;
  }
  else if (fine_tuning <= -0.5) {
    coarse_tuning -= 1.0;
    fine_tuning += 1.0;
  }

  AddSampNum(srcn, rgnAddress, 1);
  AddSimpleItem(rgnAddress + 1, 1, L"ADSR1");
  AddSimpleItem(rgnAddress + 2, 1, L"ADSR2");
  AddSimpleItem(rgnAddress + 3, 1, L"GAIN");
  AddSimpleItem(rgnAddress + 4, 1, L"Key-Off Delay");
  AddUnityKey(71 - static_cast<int> (coarse_tuning), rgnAddress + 5, 1);
  AddFineTune(static_cast<int16_t> (fine_tuning * 100.0), rgnAddress + 6, 1);
  if (instrHint.pan > 0) {
    pan = instrHint.pan / 32.0;
  }
  if (percNoteKey >= 0) {
    int8_t unityKeyCandidate = percNoteKey - (instrHint.transpose - unityKey); // correct?
    int8_t negativeUnityKeyAmount = 0;
    if (unityKeyCandidate < 0) {
      negativeUnityKeyAmount = -unityKeyCandidate;
      unityKeyCandidate = 0;
    }

    unityKey = unityKeyCandidate;
    fineTune -= negativeUnityKeyAmount * 100;
    keyLow = percNoteKey;
    keyHigh = percNoteKey;
  }
  SNESConvADSR<VGMRgn>(this, adsr1, adsr2, gain);
}

MoriSnesRgn::~MoriSnesRgn() = default;

bool MoriSnesRgn::LoadRgn() {
  return true;
}
