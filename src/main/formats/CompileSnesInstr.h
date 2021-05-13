#pragma once
#include "VGMInstrSet.h"
#include "VGMSampColl.h"
#include "VGMRgn.h"
#include "CompileSnesFormat.h"

// *******************
// CompileSnesInstrSet
// *******************

class CompileSnesInstrSet:
    public VGMInstrSet {
 public:
  CompileSnesInstrSet(RawFile *file,
                      CompileSnesVersion ver,
                      uint16_t _addrTuningTable,
                      uint16_t _addrPitchTablePtrs,
                      uint32_t _spcDirAddr,
                      const std::wstring &_name = L"CompileSnesInstrSet");
  virtual ~CompileSnesInstrSet();

  bool GetHeaderInfo() override;
  bool GetInstrPointers() override;

  CompileSnesVersion version;

 protected:
  uint16_t addrTuningTable;
  uint16_t addrPitchTablePtrs;
  uint32_t spcDirAddr;
  std::vector<uint8_t> usedSRCNs;
};

// ****************
// CompileSnesInstr
// ****************

class CompileSnesInstr
    : public VGMInstr {
 public:
  CompileSnesInstr(VGMInstrSet *instrSet,
                   CompileSnesVersion ver,
                   uint16_t addrTuningTableItem,
                   uint16_t _addrPitchTablePtrs,
                   uint8_t srcn,
                   uint32_t _spcDirAddr,
                   const std::wstring &_name = L"CompileSnesInstr");
  virtual ~CompileSnesInstr();

  bool LoadInstr() override;

  static uint32_t ExpectedSize(CompileSnesVersion version);

  CompileSnesVersion version;

 protected:
  uint16_t addrPitchTablePtrs;
  uint32_t spcDirAddr;
};

// **************
// CompileSnesRgn
// **************

class CompileSnesRgn
    : public VGMRgn {
 public:
  CompileSnesRgn(CompileSnesInstr *instr,
                 CompileSnesVersion ver,
                 uint16_t addrTuningTableItem,
                 uint16_t addrPitchTablePtrs,
                 uint8_t srcn,
                 uint32_t spcDirAddr);
  virtual ~CompileSnesRgn();

  bool LoadRgn() override;

  CompileSnesVersion version;
};
