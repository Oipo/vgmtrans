#pragma once
#include "VGMInstrSet.h"
#include "VGMSampColl.h"
#include "VGMRgn.h"

// ****************
// RareSnesInstrSet
// ****************

class RareSnesInstrSet:
    public VGMInstrSet {
 public:
  RareSnesInstrSet(RawFile *file, uint32_t offset, uint32_t _spcDirAddr, const std::wstring &_name = L"RareSnesInstrSet");
  RareSnesInstrSet(RawFile *file,
                   uint32_t offset,
                   uint32_t _spcDirAddr,
                   const std::map<uint8_t, int8_t> &_instrUnityKeyHints,
                   const std::map<uint8_t, int16_t> &_instrPitchHints,
                   const std::map<uint8_t, uint16_t> &_instrADSRHints,
                   const std::wstring &_name = L"RareSnesInstrSet");
  ~RareSnesInstrSet() override;

  virtual void Initialize();
  bool GetHeaderInfo() override;
  bool GetInstrPointers() override;

  const std::vector<uint8_t> &GetAvailableInstruments();

 protected:
  uint32_t spcDirAddr;
  uint8_t maxSRCNValue;
  std::vector<uint8_t> availInstruments;
  std::map<uint8_t, int8_t> instrUnityKeyHints;
  std::map<uint8_t, int16_t> instrPitchHints;
  std::map<uint8_t, uint16_t> instrADSRHints;

  void ScanAvailableInstruments();
};

// *************
// RareSnesInstr
// *************

class RareSnesInstr
    : public VGMInstr {
 public:
  RareSnesInstr(VGMInstrSet *instrSet,
                uint32_t offset,
                uint32_t theBank,
                uint32_t theInstrNum,
                uint32_t _spcDirAddr,
                int8_t _transpose = 0,
                int16_t _pitch = 0,
                uint16_t _adsr = 0x8FE0,
                const std::wstring &_name = L"RareSnesInstr");
  ~RareSnesInstr() override;

  bool LoadInstr() override;

 protected:
  uint32_t spcDirAddr;
  int8_t transpose;
  int16_t pitch;
  uint16_t adsr;
};

// ***********
// RareSnesRgn
// ***********

class RareSnesRgn
    : public VGMRgn {
 public:
  RareSnesRgn(RareSnesInstr *instr, uint32_t offset, int8_t _transpose = 0, int16_t _pitch = 0, uint16_t _adsr = 0x8FE0);
  ~RareSnesRgn() override;

  bool LoadRgn() override;

 protected:
  int8_t transpose;
  int16_t pitch;
  uint16_t adsr;
};
