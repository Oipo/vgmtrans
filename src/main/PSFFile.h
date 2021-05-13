#pragma once

#include <zlib.h>
#include <stdint.h>
#include "DataSeg.h"
#include "RawFile.h"

class PSFFile {
 public:
  PSFFile();
  PSFFile(RawFile *file);
  virtual ~PSFFile();

  bool Load(RawFile *file);
  bool ReadExe(uint8_t *buf, size_t len, size_t stripLen) const;
  bool ReadExeDataSeg(DataSeg *&seg, size_t len, size_t stripLen) const;
  bool Decompress(size_t decompressed_size);
  bool IsDecompressed() const;
  uint8_t GetVersion() const;
  size_t GetExeSize() const;
  size_t GetCompressedExeSize() const;
  size_t GetReservedSize() const;
  void Clear();
  const wchar_t *GetError() const;

 public:
  PSFFile *parent;
  DataSeg &exe() { return *exeData; }
  DataSeg &reserved() { return *reservedData; }
  std::map<std::string, std::string> tags;

 private:
  uint8_t version;
  DataSeg *exeData; // decompressed program section, valid only when it has been decompressed
  DataSeg *exeCompData;
  DataSeg *reservedData;
  uint32_t exeCRC;
  bool decompressed;
  const wchar_t *errorstr;
  uint8_t *stripBuf;
  size_t stripBufSize;

  int myuncompress(Bytef *dest, uLongf *destLen, z_const Bytef *source, uLong sourceLen, uLong stripLen) const;
};
