#pragma once
#include "VGMInstrSet.h"
#include "VGMSampColl.h"
#include "VGMRgn.h"
#include "PrismSnesFormat.h"

// *****************
// PrismSnesInstrSet
// *****************

class PrismSnesInstrSet:
    public VGMInstrSet {
 public:
  PrismSnesInstrSet(RawFile *file,
                    PrismSnesVersion ver,
                    uint32_t _spcDirAddr,
                    uint16_t _addrADSR1Table,
                    uint16_t _addrADSR2Table,
                    uint16_t _addrTuningTableHigh,
                    uint16_t _addrTuningTableLow,
                    const std::wstring &_name = L"PrismSnesInstrSet");
  ~PrismSnesInstrSet() override;

  bool GetHeaderInfo() override;
  bool GetInstrPointers() override;

  PrismSnesVersion version;

 protected:
  uint32_t spcDirAddr;
  uint16_t addrADSR1Table;
  uint16_t addrADSR2Table;
  uint16_t addrTuningTableHigh;
  uint16_t addrTuningTableLow;
  std::vector<uint8_t> usedSRCNs;
};

// **************
// PrismSnesInstr
// **************

class PrismSnesInstr
    : public VGMInstr {
 public:
  PrismSnesInstr(VGMInstrSet *instrSet,
                 PrismSnesVersion ver,
                 uint8_t _srcn,
                 uint32_t _spcDirAddr,
                 uint16_t _addrADSR1Entry,
                 uint16_t _addrADSR2Entry,
                 uint16_t _addrTuningEntryHigh,
                 uint16_t _addrTuningEntryLow,
                 const std::wstring &_name = L"PrismSnesInstr");
  ~PrismSnesInstr() override;

  bool LoadInstr() override;

  PrismSnesVersion version;

 protected:
  uint8_t srcn;
  uint32_t spcDirAddr;
  uint16_t addrADSR1Entry;
  uint16_t addrADSR2Entry;
  uint16_t addrTuningEntryHigh;
  uint16_t addrTuningEntryLow;
};

// ************
// PrismSnesRgn
// ************

class PrismSnesRgn
    : public VGMRgn {
 public:
  PrismSnesRgn(PrismSnesInstr *instr,
               PrismSnesVersion ver,
               uint8_t srcn,
               uint32_t spcDirAddr,
               uint16_t addrADSR1Entry,
               uint16_t addrADSR2Entry,
               uint16_t addrTuningEntryHigh,
               uint16_t addrTuningEntryLow);
  ~PrismSnesRgn() override;

  bool LoadRgn() override;

  PrismSnesVersion version;
};
