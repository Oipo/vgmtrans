#pragma once
#include "Loader.h"
#include "PSFFile.h"

class SNSFLoader:
    public VGMLoader {
 public:
  SNSFLoader();
 public:
  ~SNSFLoader() override;

  PostLoadCommand Apply(RawFile *theFile) override;
  const wchar_t *psf_read_exe(RawFile *file, unsigned char *&exebuffer, size_t &exebuffersize);
 private:
  const wchar_t *psf_read_exe_sub
      (RawFile *file, unsigned char *&exebuffer, size_t &exebuffersize, uint32_t &base_offset, bool &base_set);
  const wchar_t *load_psf_libs(PSFFile &psf,
                               RawFile *file,
                               unsigned char *&exebuffer,
                               size_t &exebuffersize,
                               uint32_t &base_offset,
                               bool &base_set);
};
