#pragma once
#include "Loader.h"

class SPCLoader:
    public VGMLoader {
 public:
  SPCLoader();
 public:
  virtual ~SPCLoader();

  virtual PostLoadCommand Apply(RawFile *theFile);
};