#pragma once
#include "Loader.h"

class PSF2Loader:
    public VGMLoader {
 public:
  PSF2Loader();
 public:
  ~PSF2Loader() override;

  PostLoadCommand Apply(RawFile *theFile) override;
  uint32_t get32lsb(const uint8_t *src);
  int psf2_decompress_block(RawFile *file,
                            unsigned fileoffset,
                            unsigned blocknumber,
                            unsigned numblocks,
                            unsigned char *decompressedblock,
                            unsigned blocksize);
  int psf2unpack(RawFile *file, unsigned long fileoffset, unsigned long dircount);
};
