#pragma once
#include "Loader.h"
#include "PSFFile.h"

class PSF1Loader:
    public VGMLoader {
 public:
  PSF1Loader();
 public:
  ~PSF1Loader() override;

  PostLoadCommand Apply(RawFile *theFile) override;
  const wchar_t *psf_read_exe(RawFile *file, unsigned char *exebuffer, unsigned exebuffersize);
 private:
  const wchar_t *load_psf_libs(PSFFile &psf, RawFile *file, unsigned char *exebuffer, unsigned exebuffersize);
};
