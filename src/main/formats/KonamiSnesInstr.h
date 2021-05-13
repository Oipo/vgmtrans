#pragma once
#include "VGMInstrSet.h"
#include "VGMSampColl.h"
#include "VGMRgn.h"
#include "KonamiSnesFormat.h"

// ******************
// KonamiSnesInstrSet
// ******************

class KonamiSnesInstrSet:
    public VGMInstrSet {
 public:
  KonamiSnesInstrSet(RawFile *file,
                     KonamiSnesVersion ver,
                     uint32_t offset,
                     uint32_t _bankedInstrOffset,
                     uint8_t _firstBankedInstr,
                     uint32_t _percInstrOffset,
                     uint32_t _spcDirAddr,
                     const std::wstring &_name = L"KonamiSnesInstrSet");
  ~KonamiSnesInstrSet() override;

  bool GetHeaderInfo() override;
  bool GetInstrPointers() override;

  KonamiSnesVersion version;

 protected:
  uint32_t bankedInstrOffset;
  uint8_t firstBankedInstr;
  uint32_t percInstrOffset;
  uint32_t spcDirAddr;
  std::vector<uint8_t> usedSRCNs;
};

// ***************
// KonamiSnesInstr
// ***************

class KonamiSnesInstr
    : public VGMInstr {
 public:
  KonamiSnesInstr(VGMInstrSet *instrSet,
                  KonamiSnesVersion ver,
                  uint32_t offset,
                  uint32_t theBank,
                  uint32_t theInstrNum,
                  uint32_t _spcDirAddr,
                  bool _percussion,
                  const std::wstring &_name = L"KonamiSnesInstr");
  ~KonamiSnesInstr() override;

  bool LoadInstr() override;

  static bool IsValidHeader
      (RawFile *file, KonamiSnesVersion version, uint32_t addrInstrHeader, uint32_t spcDirAddr, bool validateSample);
  static uint32_t ExpectedSize(KonamiSnesVersion version);

  KonamiSnesVersion version;

 protected:
  uint32_t spcDirAddr;
  bool percussion;
};

// *************
// KonamiSnesRgn
// *************

class KonamiSnesRgn
    : public VGMRgn {
 public:
  KonamiSnesRgn(KonamiSnesInstr *instr, KonamiSnesVersion ver, uint32_t offset, bool percussion);
  ~KonamiSnesRgn() override;

  bool LoadRgn() override;

  KonamiSnesVersion version;
};
