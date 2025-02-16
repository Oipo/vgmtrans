#pragma once
#include "Scanner.h"
#include "BytePattern.h"

enum KonamiSnesVersion: uint8_t; // see KonamiSnesFormat.h

class KonamiSnesScanner:
    public VGMScanner {
 public:
  KonamiSnesScanner() {
    USE_EXTENSION(L"spc");
  }
  ~KonamiSnesScanner() override = default;

  void Scan(RawFile *file, void *info = 0) override;
  void SearchForKonamiSnesFromARAM(RawFile *file);
  void SearchForKonamiSnesFromROM(RawFile *file);

 private:
  static BytePattern ptnSetSongHeaderAddressGG4;
  static BytePattern ptnReadSongListPNTB;
  static BytePattern ptnReadSongListAXE;
  static BytePattern ptnReadSongListCNTR3;
  static BytePattern ptnJumpToVcmdGG4;
  static BytePattern ptnJumpToVcmdCNTR3;
  static BytePattern ptnBranchForVcmd6xMDR2;
  static BytePattern ptnBranchForVcmd6xCNTR3;
  static BytePattern ptnSetDIRGG4;
  static BytePattern ptnSetDIRCNTR3;
  static BytePattern ptnLoadInstrJOP;
  static BytePattern ptnLoadInstrGP;
  static BytePattern ptnLoadInstrGG4;
  static BytePattern ptnLoadInstrPNTB;
  static BytePattern ptnLoadInstrCNTR3;
  static BytePattern ptnLoadPercInstrGG4;
};