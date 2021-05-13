#pragma once
#include "VGMInstrSet.h"
#include "VGMSampColl.h"
#include "VGMRgn.h"
#include "GraphResSnesFormat.h"

// ********************
// GraphResSnesInstrSet
// ********************

class GraphResSnesInstrSet:
    public VGMInstrSet {
 public:
  GraphResSnesInstrSet(RawFile *file,
                       GraphResSnesVersion ver,
                       uint32_t _spcDirAddr,
                       const std::map<uint8_t, uint16_t> &_instrADSRHints = std::map<uint8_t, uint16_t>(),
                       const std::wstring &_name = L"GraphResSnesInstrSet");
  ~GraphResSnesInstrSet() override;

  bool GetHeaderInfo() override;
  bool GetInstrPointers() override;

  GraphResSnesVersion version;

 protected:
  uint32_t spcDirAddr;
  std::map<uint8_t, uint16_t> instrADSRHints;
  std::vector<uint8_t> usedSRCNs;
};

// *****************
// GraphResSnesInstr
// *****************

class GraphResSnesInstr
    : public VGMInstr {
 public:
  GraphResSnesInstr(VGMInstrSet *instrSet,
                    GraphResSnesVersion ver,
                    uint8_t srcn,
                    uint32_t _spcDirAddr,
                    uint16_t _adsr = 0x8fe0,
                    const std::wstring &_name = L"GraphResSnesInstr");
  ~GraphResSnesInstr() override;

  bool LoadInstr() override;

  GraphResSnesVersion version;

 protected:
  uint32_t spcDirAddr;
  uint16_t adsr;
};

// ***************
// GraphResSnesRgn
// ***************

class GraphResSnesRgn
    : public VGMRgn {
 public:
  GraphResSnesRgn
      (GraphResSnesInstr *instr, GraphResSnesVersion ver, uint8_t srcn, uint32_t spcDirAddr, uint16_t adsr = 0x8fe0);
  ~GraphResSnesRgn() override;

  bool LoadRgn() override;

  GraphResSnesVersion version;
};
