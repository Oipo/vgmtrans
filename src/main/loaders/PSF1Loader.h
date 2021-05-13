#pragma once
#include "Loader.h"
#include "PSFFile.h"

class PSF1Loader:
    public VGMLoader {
 public:
  PSF1Loader();
 public:
  virtual ~PSF1Loader();

  virtual PostLoadCommand Apply(RawFile *theFile);
  const wchar_t *psf_read_exe(RawFile *file, unsigned char *exebuffer, unsigned exebuffersize);
 private:
  const wchar_t *load_psf_libs(PSFFile &psf, RawFile *file, unsigned char *exebuffer, unsigned exebuffersize);
};
