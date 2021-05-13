#pragma once
#include "VGMInstrSet.h"
#include "VGMSampColl.h"
#include "VGMRgn.h"
#include "FalcomSnesFormat.h"

class FalcomSnesInstrSet;
class FalcomSnesInstr;
class FalcomSnesRgn;

// ******************
// FalcomSnesInstrSet
// ******************

class FalcomSnesInstrSet:
    public VGMInstrSet {
 public:
  friend FalcomSnesInstr;
  friend FalcomSnesRgn;

  FalcomSnesInstrSet(RawFile *file,
                     FalcomSnesVersion ver,
                     uint32_t offset,
                     uint32_t _addrSampToInstrTable,
                     uint32_t _spcDirAddr,
                     const std::map<uint8_t, uint16_t> &_instrADSRHints,
                     const std::wstring &_name = L"FalcomSnesInstrSet");
  virtual ~FalcomSnesInstrSet();

  bool GetHeaderInfo() override;
  bool GetInstrPointers() override;

  FalcomSnesVersion version;

 protected:
  uint32_t spcDirAddr;
  uint32_t addrSampToInstrTable;
  std::vector<uint8_t> usedSRCNs;
  std::map<uint8_t, uint16_t> instrADSRHints;
};

// *************
// FalcomSnesInstr
// *************

class FalcomSnesInstr
    : public VGMInstr {
 public:
  FalcomSnesInstr(VGMInstrSet *instrSet,
                  FalcomSnesVersion ver,
                  uint32_t offset,
                  uint32_t theBank,
                  uint32_t theInstrNum,
                  uint8_t _srcn,
                  uint32_t _spcDirAddr,
                  const std::wstring &_name = L"FalcomSnesInstr");
  ~FalcomSnesInstr() override;

  bool LoadInstr() override;

  static bool IsValidHeader
      (RawFile *file, FalcomSnesVersion version, uint32_t addrInstrHeader, uint32_t spcDirAddr, bool validateSample);

  FalcomSnesVersion version;

 protected:
  uint8_t srcn;
  uint32_t spcDirAddr;
};

// ***********
// FalcomSnesRgn
// ***********

class FalcomSnesRgn
    : public VGMRgn {
 public:
  FalcomSnesRgn(FalcomSnesInstr *instr,
             FalcomSnesVersion ver,
             uint32_t offset,
             uint8_t srcn);
  ~FalcomSnesRgn() override;

  bool LoadRgn() override;

  FalcomSnesVersion version;
};
