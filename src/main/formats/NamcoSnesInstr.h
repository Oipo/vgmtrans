#pragma once
#include "VGMInstrSet.h"
#include "VGMSampColl.h"
#include "VGMRgn.h"
#include "NamcoSnesFormat.h"

// *****************
// NamcoSnesInstrSet
// *****************

class NamcoSnesInstrSet:
    public VGMInstrSet {
 public:
  NamcoSnesInstrSet(RawFile *file,
                    NamcoSnesVersion ver,
                    uint32_t _spcDirAddr,
                    uint16_t _addrTuningTable,
                    const std::wstring &_name = L"NamcoSnesInstrSet");
  ~NamcoSnesInstrSet() override;

  bool GetHeaderInfo() override;
  bool GetInstrPointers() override;

  NamcoSnesVersion version;

 protected:
  uint32_t spcDirAddr;
  uint16_t addrTuningTable;
  std::vector<uint8_t> usedSRCNs;
};

// **************
// NamcoSnesInstr
// **************

class NamcoSnesInstr
    : public VGMInstr {
 public:
  NamcoSnesInstr(VGMInstrSet *instrSet,
                 NamcoSnesVersion ver,
                 uint8_t srcn,
                 uint32_t _spcDirAddr,
                 uint16_t _addrTuningEntry,
                 const std::wstring &_name = L"NamcoSnesInstr");
  ~NamcoSnesInstr() override;

  bool LoadInstr() override;

  NamcoSnesVersion version;

 protected:
  uint32_t spcDirAddr;
  uint16_t addrTuningEntry;
};

// ************
// NamcoSnesRgn
// ************

class NamcoSnesRgn
    : public VGMRgn {
 public:
  NamcoSnesRgn
      (NamcoSnesInstr *instr, NamcoSnesVersion ver, uint8_t srcn, uint32_t spcDirAddr, uint16_t addrTuningEntry);
  ~NamcoSnesRgn() override;

  bool LoadRgn() override;

  NamcoSnesVersion version;
};
