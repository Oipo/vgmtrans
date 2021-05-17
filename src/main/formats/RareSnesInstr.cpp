#include "pch.h"
#include "RareSnesInstr.h"
#include "Format.h"
#include "SNESDSP.h"
#include "RareSnesFormat.h"

// ****************
// RareSnesInstrSet
// ****************

RareSnesInstrSet::RareSnesInstrSet(RawFile *file, uint32_t offset, uint32_t _spcDirAddr, const std::wstring &_name) :
    VGMInstrSet(RareSnesFormat::name, file, offset, 0, _name),
    spcDirAddr(_spcDirAddr),
    maxSRCNValue(255) {
  Initialize();
}

RareSnesInstrSet::RareSnesInstrSet(RawFile *file,
                                   uint32_t offset,
                                   uint32_t _spcDirAddr,
                                   const std::map<uint8_t, int8_t> &_instrUnityKeyHints,
                                   const std::map<uint8_t, int16_t> &_instrPitchHints,
                                   const std::map<uint8_t, uint16_t> &_instrADSRHints,
                                   const std::wstring &_name) :
    VGMInstrSet(RareSnesFormat::name, file, offset, 0, _name),
    spcDirAddr(_spcDirAddr),
    maxSRCNValue(255),
    instrUnityKeyHints(_instrUnityKeyHints),
    instrPitchHints(_instrPitchHints),
    instrADSRHints(_instrADSRHints) {
  Initialize();
}

RareSnesInstrSet::~RareSnesInstrSet() = default;

void RareSnesInstrSet::Initialize() {
  for (uint32_t srcn = 0; srcn < 256; srcn++) {
    uint32_t offDirEnt = spcDirAddr + (srcn * 4);
    if (offDirEnt + 4 > 0x10000) {
      maxSRCNValue = srcn - 1;
      break;
    }

    if (GetShort(offDirEnt) == 0) {
      maxSRCNValue = srcn - 1;
      break;
    }
  }

  unLength = 0x100;
  if (dwOffset + unLength > GetRawFile()->size()) {
    unLength = GetRawFile()->size() - dwOffset;
  }
  ScanAvailableInstruments();
}

void RareSnesInstrSet::ScanAvailableInstruments() {
  availInstruments.clear();

  bool firstZero = true;
  for (uint32_t inst = 0; inst < unLength; inst++) {
    uint8_t srcn = GetByte(dwOffset + inst);

    if (srcn == 0) {
      if (firstZero) {
        firstZero = false;
      }
      else {
        continue;
      }
    }

    uint32_t offDirEnt = spcDirAddr + (srcn * 4);
    if (offDirEnt + 4 > 0x10000) {
      continue;
    }
    if (srcn > maxSRCNValue) {
      continue;
    }

    if (!SNESSampColl::IsValidSampleDir(rawfile, offDirEnt, true)) {
      continue;
    }

    uint16_t addrSampStart = GetShort(offDirEnt);
    //uint16_t addrSampLoop = GetShort(offDirEnt + 2);
    // not in DIR table
    if (addrSampStart < spcDirAddr + (inst * 4)) {
      continue;
    }
    // address 0 is probably legit, but it should not be used
    if (addrSampStart == 0) {
      continue;
    }
    // Rare engine does not break the following rule... perhaps
    if (addrSampStart < spcDirAddr) {
      continue;
    }

    availInstruments.push_back(inst);
  }
}

bool RareSnesInstrSet::GetHeaderInfo() {
  return true;
}

bool RareSnesInstrSet::GetInstrPointers() {
  // make a loopup table for duplicated instruments,
  // to give them appropriate instrument metadata
  // (it's a workaround for wrong instrument tuning)
  std::map<uint16_t, uint8_t> addrToInstrLookups;
  for (auto & instrUnityKeyHint : instrUnityKeyHints) {
    uint8_t inst = instrUnityKeyHint.first;
    uint8_t srcn = GetByte(dwOffset + inst);

    uint32_t offDirEnt = spcDirAddr + (srcn * 4);
    if (addrToInstrLookups.count(offDirEnt) == 0) {
      addrToInstrLookups[offDirEnt] = inst;
    }
  }

  for (unsigned char inst : availInstruments) {
    uint8_t srcn = GetByte(dwOffset + inst);

    uint8_t inst_remapped = inst;
    uint32_t offDirEnt = spcDirAddr + (srcn * 4);
    if (addrToInstrLookups.count(offDirEnt) != 0) {
      inst_remapped = addrToInstrLookups[offDirEnt];
    }

    int8_t transpose = 0;
    std::map<uint8_t, int8_t>::iterator itrKey;
    itrKey = this->instrUnityKeyHints.find(inst_remapped);
    if (itrKey != instrUnityKeyHints.end()) {
      transpose = itrKey->second;
    }

    int16_t pitch = 0;
    std::map<uint8_t, int16_t>::iterator itrPitch;
    itrPitch = this->instrPitchHints.find(inst_remapped);
    if (itrPitch != instrPitchHints.end()) {
      pitch = itrPitch->second;
    }

    uint16_t adsr = 0x8FE0;
    std::map<uint8_t, uint16_t>::iterator itrADSR;
    itrADSR = this->instrADSRHints.find(inst_remapped);
    if (itrADSR != instrADSRHints.end()) {
      adsr = itrADSR->second;
    }

    std::wostringstream instrName;
    instrName << L"Instrument " << inst;
    RareSnesInstr *newInstr = new RareSnesInstr(this,
                                                dwOffset + inst,
                                                inst >> 7,
                                                inst & 0x7f,
                                                spcDirAddr,
                                                transpose,
                                                pitch,
                                                adsr,
                                                instrName.str());
    aInstrs.push_back(newInstr);
  }
  return !aInstrs.empty();
}

const std::vector<uint8_t> &RareSnesInstrSet::GetAvailableInstruments() {
  return availInstruments;
}

// *************
// RareSnesInstr
// *************

RareSnesInstr::RareSnesInstr(VGMInstrSet *instrSet,
                             uint32_t offset,
                             uint32_t theBank,
                             uint32_t theInstrNum,
                             uint32_t _spcDirAddr,
                             int8_t _transpose,
                             int16_t _pitch,
                             uint16_t _adsr,
                             const std::wstring &_name) :
    VGMInstr(instrSet, offset, 1, theBank, theInstrNum, _name),
    spcDirAddr(_spcDirAddr),
    transpose(_transpose),
    pitch(_pitch),
    adsr(_adsr) {
}

RareSnesInstr::~RareSnesInstr() = default;

bool RareSnesInstr::LoadInstr() {
  uint8_t srcn = GetByte(dwOffset);
  uint32_t offDirEnt = spcDirAddr + (srcn * 4);
  if (offDirEnt + 4 > 0x10000) {
    return false;
  }

  uint16_t addrSampStart = GetShort(offDirEnt);

  RareSnesRgn *rgn = new RareSnesRgn(this, dwOffset, transpose, pitch, adsr);
  rgn->sampOffset = addrSampStart - spcDirAddr;
  aRgns.push_back(rgn);
  return true;
}

// ***********
// RareSnesRgn
// ***********

RareSnesRgn::RareSnesRgn(RareSnesInstr *instr, uint32_t offset, int8_t _transpose, int16_t _pitch, uint16_t _adsr) :
    VGMRgn(instr, offset, 1),
    transpose(_transpose),
    pitch(_pitch),
    adsr(_adsr) {
  // normalize (it is needed especially since SF2 pitch correction is signed 8-bit)
  int16_t pitchKeyShift = (_pitch / 100);
  int8_t realTranspose = _transpose + pitchKeyShift;
  int16_t realPitch = _pitch - (pitchKeyShift * 100);

  // NOTE_PITCH_TABLE[73] == 0x1000
  // 0x80 + (73 - 36) = 0xA5
  SetUnityKey(36 + 36 - realTranspose);
  SetFineTune(realPitch);
  SNESConvADSR<VGMRgn>(this, _adsr >> 8, _adsr & 0xff, 0);
}

RareSnesRgn::~RareSnesRgn() = default;

bool RareSnesRgn::LoadRgn() {
  return true;
}
