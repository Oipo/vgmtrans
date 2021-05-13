#pragma once
#include "Scanner.h"

class OrgScanner:
    public VGMScanner {
 public:
  OrgScanner();
  virtual ~OrgScanner();

  virtual void Scan(RawFile *file, void *info = 0);
  void SearchForOrgSeq(RawFile *file);
};
