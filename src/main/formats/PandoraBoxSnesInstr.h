#pragma once
#include "VGMInstrSet.h"
#include "VGMSampColl.h"
#include "VGMRgn.h"
#include "PandoraBoxSnesFormat.h"

// **********************
// PandoraBoxSnesInstrSet
// **********************

class PandoraBoxSnesInstrSet:
    public VGMInstrSet {
 public:
  PandoraBoxSnesInstrSet(RawFile *file,
                         PandoraBoxSnesVersion ver,
                         uint32_t spcDirAddr,
                         uint16_t addrLocalInstrTable,
                         uint16_t addrGlobalInstrTable,
                         uint8_t globalInstrumentCount,
                         const std::map<uint8_t, uint16_t> &instrADSRHints = std::map<uint8_t, uint16_t>(),
                         const std::wstring &name = L"PandoraBoxSnesInstrSet");
  virtual ~PandoraBoxSnesInstrSet();

  bool GetHeaderInfo() override;
  bool GetInstrPointers() override;

  PandoraBoxSnesVersion version;

 protected:
  uint32_t spcDirAddr;
  uint16_t addrLocalInstrTable;
  uint16_t addrGlobalInstrTable;
  uint8_t globalInstrumentCount;
  std::vector<uint8_t> globalInstrTable;
  std::map<uint8_t, uint16_t> instrADSRHints;
  std::vector<uint8_t> usedSRCNs;
};

// *******************
// PandoraBoxSnesInstr
// *******************

class PandoraBoxSnesInstr
    : public VGMInstr {
 public:
  PandoraBoxSnesInstr(VGMInstrSet *instrSet,
                      PandoraBoxSnesVersion ver,
                      uint32_t offset,
                      uint8_t theInstrNum,
                      uint8_t srcn,
                      uint32_t spcDirAddr,
                      uint16_t adsr = 0x8fe0,
                      const std::wstring &name = L"PandoraBoxSnesInstr");
  virtual ~PandoraBoxSnesInstr();

  bool LoadInstr() override;

  PandoraBoxSnesVersion version;

 protected:
  uint32_t spcDirAddr;
  uint8_t srcn;
  uint16_t adsr;
};

// *****************
// PandoraBoxSnesRgn
// *****************

class PandoraBoxSnesRgn
    : public VGMRgn {
 public:
  PandoraBoxSnesRgn(PandoraBoxSnesInstr *instr,
                    PandoraBoxSnesVersion ver,
                    uint32_t offset,
                    uint8_t srcn,
                    uint32_t spcDirAddr,
                    uint16_t adsr = 0x8fe0);
  virtual ~PandoraBoxSnesRgn();

  bool LoadRgn() override;

  PandoraBoxSnesVersion version;
};
