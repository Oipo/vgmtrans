#pragma once
#include "common.h"
#include "HOSAFormat.h"
#include "VGMInstrSet.h"
#include "VGMSampColl.h"



// *****************
// HOSAInstrSet
// *****************

class HOSAInstrSet
    : public VGMInstrSet {

 public:
  HOSAInstrSet(RawFile *file, uint32_t offset);
  ~HOSAInstrSet() override;

  bool GetHeaderInfo() override;
  bool GetInstrPointers() override;

 public:

  struct InstrHeader {
    char strHeader[8];
    uint32_t numInstr;
  };

 public:
  InstrHeader instrheader;
};


// **************
// HOSAInstr
// **************

class HOSAInstr
    : public VGMInstr {
 public:

  struct InstrInfo {
    uint32_t numRgns;
  };

  struct RgnInfo {
    uint32_t sampOffset;
    uint8_t volume;           //percent volume 0-0xFF
    uint8_t note_range_high;
    uint8_t iSemiToneTune;    //unity key
    uint8_t iFineTune;        //unknown - definitely not finetune
    uint8_t ADSR_unk;         //the nibbles get read individually.  Conditional code related to this gets 0'd out in PSF file
                              //I disassembled (removed during optimization), so I can't see what it does. probably determines
                              //Sm and Sd, so not terribly important.
    uint8_t ADSR_Am;          // Determines ADSR Attack Mode value.
    uint8_t unk_A;
    uint8_t iPan;             //pan 0x80 - hard left    0xFF - hard right.  anything below results in center (but may be undefined)
    uint32_t ADSR_vals;       //The ordering is all messed up.  The code which loads these values is at 8007D8EC
  };


 public:
  HOSAInstr(VGMInstrSet *instrSet, uint32_t offset, uint32_t length, uint32_t theBank, uint32_t theInstrNum);
  ~HOSAInstr() override { delete[] rgns; }
  bool LoadInstr() override;

 public:
  InstrInfo instrinfo;
  RgnInfo *rgns;
};
