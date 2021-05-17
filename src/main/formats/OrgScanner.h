#pragma once
#include "Scanner.h"

class OrgScanner:
    public VGMScanner {
 public:
  OrgScanner();
  ~OrgScanner() override;

  void Scan(RawFile *file, void *info = 0) override;
  void SearchForOrgSeq(RawFile *file);
};
