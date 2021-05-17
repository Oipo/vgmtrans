#pragma once
#include "VGMInstrSet.h"
#include "VGMSampColl.h"
#include "VGMRgn.h"
#include "SuzukiSnesFormat.h"

// ******************
// SuzukiSnesInstrSet
// ******************

class SuzukiSnesInstrSet:
    public VGMInstrSet {
 public:
  SuzukiSnesInstrSet(RawFile *file,
                     SuzukiSnesVersion ver,
                     uint32_t _spcDirAddr,
                     uint16_t _addrSRCNTable,
                     uint16_t _addrVolumeTable,
                     uint16_t _addrADSRTable,
                     uint16_t _addrTuningTable,
                     const std::wstring &_name = L"SuzukiSnesInstrSet");
  ~SuzukiSnesInstrSet() override;

  bool GetHeaderInfo() override;
  bool GetInstrPointers() override;

  SuzukiSnesVersion version;

 protected:
  uint32_t spcDirAddr;
  uint16_t addrSRCNTable;
  uint16_t addrVolumeTable;
  uint16_t addrTuningTable;
  uint16_t addrADSRTable;
  std::vector<uint8_t> usedSRCNs;
};

// *************
// SuzukiSnesInstr
// *************

class SuzukiSnesInstr
    : public VGMInstr {
 public:
  SuzukiSnesInstr(VGMInstrSet *instrSet,
                  SuzukiSnesVersion ver,
                  uint8_t _instrNum,
                  uint32_t _spcDirAddr,
                  uint16_t _addrSRCNTable,
                  uint16_t _addrVolumeTable,
                  uint16_t _addrADSRTable,
                  uint16_t _addrTuningTable,
                  const std::wstring &_name = L"SuzukiSnesInstr");
  ~SuzukiSnesInstr() override;

  bool LoadInstr() override;

  SuzukiSnesVersion version;

 protected:
  uint32_t spcDirAddr;
  uint16_t addrSRCNTable;
  uint16_t addrVolumeTable;
  uint16_t addrTuningTable;
  uint16_t addrADSRTable;
};

// *************
// SuzukiSnesRgn
// *************

class SuzukiSnesRgn
    : public VGMRgn {
 public:
  SuzukiSnesRgn(SuzukiSnesInstr *instr,
                SuzukiSnesVersion ver,
                uint8_t instrNum,
                uint32_t spcDirAddr,
                uint16_t addrSRCNTable,
                uint16_t addrVolumeTable,
                uint16_t addrADSRTable,
                uint16_t addrTuningTable);
  ~SuzukiSnesRgn() override;

  bool LoadRgn() override;

  SuzukiSnesVersion version;
};
