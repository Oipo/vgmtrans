#pragma once
#include "VGMInstrSet.h"
#include "VGMSampColl.h"
#include "VGMRgn.h"
#include "HudsonSnesFormat.h"

// ******************
// HudsonSnesInstrSet
// ******************

class HudsonSnesInstrSet:
    public VGMInstrSet {
 public:
  HudsonSnesInstrSet(RawFile *file,
                     HudsonSnesVersion ver,
                     uint32_t offset,
                     uint32_t length,
                     uint32_t _spcDirAddr,
                     uint32_t _addrSampTuningTable,
                     const std::wstring &_name = L"HudsonSnesInstrSet");
  ~HudsonSnesInstrSet() override;

  bool GetHeaderInfo() override;
  bool GetInstrPointers() override;

  HudsonSnesVersion version;

 protected:
  uint32_t spcDirAddr;
  uint32_t addrSampTuningTable;
  std::vector<uint8_t> usedSRCNs;
};

// ***************
// HudsonSnesInstr
// ***************

class HudsonSnesInstr
    : public VGMInstr {
 public:
  HudsonSnesInstr(VGMInstrSet *instrSet,
                  HudsonSnesVersion ver,
                  uint32_t offset,
                  uint8_t instrNum,
                  uint32_t _spcDirAddr,
                  uint32_t _addrSampTuningTable,
                  const std::wstring &_name = L"HudsonSnesInstr");
  ~HudsonSnesInstr() override;

  bool LoadInstr() override;

  HudsonSnesVersion version;

 protected:
  uint32_t spcDirAddr;
  uint32_t addrSampTuningTable;
};

// *************
// HudsonSnesRgn
// *************

class HudsonSnesRgn
    : public VGMRgn {
 public:
  HudsonSnesRgn
      (HudsonSnesInstr *instr, HudsonSnesVersion ver, uint32_t offset, uint32_t spcDirAddr, uint32_t addrTuningEntry);
  ~HudsonSnesRgn() override;

  bool LoadRgn() override;

  HudsonSnesVersion version;
};
