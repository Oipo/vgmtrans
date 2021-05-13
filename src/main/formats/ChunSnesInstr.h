#pragma once
#include "VGMInstrSet.h"
#include "VGMSampColl.h"
#include "VGMRgn.h"
#include "ChunSnesFormat.h"

// ****************
// ChunSnesInstrSet
// ****************

class ChunSnesInstrSet:
    public VGMInstrSet {
 public:
  ChunSnesInstrSet(RawFile *file,
                   ChunSnesVersion ver,
                   uint16_t addrInstrSetTable,
                   uint16_t _addrSampNumTable,
                   uint16_t _addrSampleTable,
                   uint32_t _spcDirAddr,
                   const std::wstring &_name = L"ChunSnesInstrSet");
  virtual ~ChunSnesInstrSet();

  bool GetHeaderInfo() override;
  bool GetInstrPointers() override;

  ChunSnesVersion version;

 protected:
  uint32_t spcDirAddr;
  uint16_t addrSampNumTable;
  uint16_t addrSampleTable;
  std::vector<uint8_t> usedSRCNs;
};

// *************
// ChunSnesInstr
// *************

class ChunSnesInstr
    : public VGMInstr {
 public:
  ChunSnesInstr(VGMInstrSet *instrSet,
                ChunSnesVersion ver,
                uint8_t theInstrNum,
                uint16_t addrInstr,
                uint16_t _addrSampleTable,
                uint32_t _spcDirAddr,
                const std::wstring &_name = L"ChunSnesInstr");
  virtual ~ChunSnesInstr();

  bool LoadInstr() override;

  ChunSnesVersion version;

 protected:
  uint16_t addrSampleTable;
  uint32_t spcDirAddr;
};

// ***********
// ChunSnesRgn
// ***********

class ChunSnesRgn
    : public VGMRgn {
 public:
  ChunSnesRgn(ChunSnesInstr *instr, ChunSnesVersion ver, uint8_t srcn, uint16_t addrRgn, uint32_t spcDirAddr);
  virtual ~ChunSnesRgn();

  bool LoadRgn() override;

  ChunSnesVersion version;
};
