#pragma once
#include "VGMInstrSet.h"
#include "VGMSampColl.h"
#include "VGMRgn.h"
#include "TamSoftPS1Format.h"

// ******************
// TamSoftPS1InstrSet
// ******************

class TamSoftPS1InstrSet:
    public VGMInstrSet {
 public:
  TamSoftPS1InstrSet(RawFile *file, uint32_t offset, bool ps2, const std::wstring &name = L"TamSoftPS1InstrSet");
  virtual ~TamSoftPS1InstrSet();

  bool GetHeaderInfo() override;
  bool GetInstrPointers() override;

  bool ps2;
};

// ***************
// TamSoftPS1Instr
// ***************

class TamSoftPS1Instr
    : public VGMInstr {
 public:
  TamSoftPS1Instr(TamSoftPS1InstrSet *instrSet, uint8_t instrNum, const std::wstring &name = L"TamSoftPS1Instr");
  virtual ~TamSoftPS1Instr();

  bool LoadInstr() override;

 protected:
  uint32_t spcDirAddr;
  uint32_t addrSampTuningTable;
};

// *************
// TamSoftPS1Rgn
// *************

class TamSoftPS1Rgn
    : public VGMRgn {
 public:
  TamSoftPS1Rgn(TamSoftPS1Instr *instr, uint32_t offset, bool ps2);
  virtual ~TamSoftPS1Rgn();

  bool LoadRgn() override;
};
