#pragma once
#include "VGMInstrSet.h"
#include "VGMSampColl.h"
#include "VGMRgn.h"

// ****************
// CapcomSnesInstrSet
// ****************

class CapcomSnesInstrSet:
    public VGMInstrSet {
 public:
  CapcomSnesInstrSet
      (RawFile *file, uint32_t offset, uint32_t _spcDirAddr, const std::wstring &_name = L"CapcomSnesInstrSet");
  virtual ~CapcomSnesInstrSet();

  bool GetHeaderInfo() override;
  bool GetInstrPointers() override;

 protected:
  uint32_t spcDirAddr;
  std::vector<uint8_t> usedSRCNs;
};

// *************
// CapcomSnesInstr
// *************

class CapcomSnesInstr
    : public VGMInstr {
 public:
  CapcomSnesInstr(VGMInstrSet *instrSet,
                  uint32_t offset,
                  uint32_t theBank,
                  uint32_t theInstrNum,
                  uint32_t _spcDirAddr,
                  const std::wstring &_name = L"CapcomSnesInstr");
  virtual ~CapcomSnesInstr();

  bool LoadInstr() override;

  static bool IsValidHeader(RawFile *file, uint32_t addrInstrHeader, uint32_t spcDirAddr, bool validateSample);

 protected:
  uint32_t spcDirAddr;
};

// ***********
// CapcomSnesRgn
// ***********

class CapcomSnesRgn
    : public VGMRgn {
 public:
  CapcomSnesRgn(CapcomSnesInstr *instr, uint32_t offset);
  virtual ~CapcomSnesRgn();

  bool LoadRgn() override;
};
