#pragma once
#include "Loader.h"

class SPCLoader:
    public VGMLoader {
 public:
  SPCLoader();
 public:
  ~SPCLoader() override;

  PostLoadCommand Apply(RawFile *theFile) override;
};