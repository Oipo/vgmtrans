#pragma once
#include "Format.h"

#include <utility>
#include "Root.h"
#include "SonyPS2Scanner.h"
#include "Matcher.h"
#include "VGMColl.h"

struct VersCk {
  uint32_t Creator;
  uint32_t Type;
  uint32_t chunkSize;
  uint16_t reserved;
  uint8_t versionMajor;
  uint8_t versionMinor;
};

// ***********
// SonyPS2Coll
// ***********

class SonyPS2Coll:
    public VGMColl {
 public:
  SonyPS2Coll(std::wstring _name = L"Unnamed Collection") : VGMColl(std::move(_name)) { }
  ~SonyPS2Coll() override = default;
};


// *************
// SonyPS2Format
// *************

BEGIN_FORMAT(SonyPS2)
  USING_SCANNER(SonyPS2Scanner)
  USING_MATCHER_WITH_ARG(FilenameMatcher, true)
  USING_COLL(SonyPS2Coll)
END_FORMAT()


