#include "pch.h"

#include <zlib.h>
#include "Root.h"
#include "PSFFile.h"

using namespace std;

#define PSF_TAG_SIG             "[TAG]"
#define PSF_TAG_SIG_LEN         5
#define PSF_STRIP_BUF_SIZE      4096

PSFFile::PSFFile() :
    stripBuf(nullptr),
    stripBufSize(PSF_STRIP_BUF_SIZE) {
  stripBuf = new uint8_t[PSF_STRIP_BUF_SIZE];

  exeData = new DataSeg();
  exeCompData = new DataSeg();
  reservedData = new DataSeg();

  Clear();
}

PSFFile::PSFFile(RawFile *file) :
    stripBuf(nullptr),
    stripBufSize(PSF_STRIP_BUF_SIZE) {
  stripBuf = new uint8_t[PSF_STRIP_BUF_SIZE];
  stripBufSize = PSF_STRIP_BUF_SIZE;

  exeData = new DataSeg();
  exeCompData = new DataSeg();
  reservedData = new DataSeg();

  Load(file);
}

PSFFile::~PSFFile() {
  delete exeData;
  delete exeCompData;
  delete reservedData;
  delete[] stripBuf;
}

bool PSFFile::Load(RawFile *file) {
  Clear();

  // Check file size.
  uint32_t fileSize = file->size();
  if (fileSize < 0x10) {
    errorstr = L"PSF too small - likely corrupt";
    return false;
  }

  // Check PSF signature.
  uint8_t psfSig[4];
  file->GetBytes(0, 4, psfSig);
  if (memcmp(psfSig, "PSF", 3) != 0) {
    errorstr = L"Invalid PSF signature";
    return false;
  }

  // Read the version number.
  version = psfSig[3];

  // Read the remaining header values.
  uint32_t reservedSize = file->GetWord(4);
  uint32_t exeSize = file->GetWord(8);
  exeCRC = file->GetWord(12);

  // Consistency check on section lengths.
  if ((reservedSize > fileSize) ||
      (exeSize > fileSize) ||
      ((16 + reservedSize + exeSize) > fileSize)) {
    errorstr = L"PSF header is inconsistent";
    return false;
  }

  // Read compressed program section.
  uint8_t *zexebuf = new uint8_t[exeSize > 0 ? exeSize : 1];
  if (zexebuf == nullptr) {
    errorstr = L"Out of memory reading the file";
    return false;
  }
  file->GetBytes(16 + reservedSize, exeSize, zexebuf);
  exeCompData->load(zexebuf, 0, exeSize);

  // Check program section CRC.
  if (exeCRC != crc32(crc32(0L, nullptr, 0), zexebuf, exeSize)) {
    errorstr = L"CRC failure - executable data is corrupt";
    exeCompData->clear();
    return false;
  }

  // Read reserved section.
  uint8_t *reservebuf = new uint8_t[reservedSize ? reservedSize : 1];
  if (reservebuf == nullptr) {
    errorstr = L"Out of memory reading the file";
    exeCompData->clear();
    return false;
  }
  file->GetBytes(16, reservedSize, reservebuf);
  reservedData->load(reservebuf, 0, reservedSize);

  // Check existence of tag section.
  uint32_t tagSectionOffset = 16 + reservedSize + exeSize;
  uint32_t tagSectionSize = fileSize - tagSectionOffset;
/*  bool hasTagSection = false;
  if (tagSectionSize >= PSF_TAG_SIG_LEN) {
    uint8_t tagSig[5];
    file->GetBytes(tagSectionOffset, PSF_TAG_SIG_LEN, tagSig);
    if (memcmp(tagSig, PSF_TAG_SIG, PSF_TAG_SIG_LEN) == 0) {
      hasTagSection = true;
    }
  }*/

  // Check if tag field exists.
  char *tagSect = new char[tagSectionSize + 1];
  if (tagSect == nullptr) {
    errorstr = L"Out of memory reading the file";
    exeCompData->clear();
    reservedData->clear();
    return false;
  }
  file->GetBytes(tagSectionOffset, tagSectionSize, tagSect);
  tagSect[tagSectionSize] = '\0';

  // Parse tag section. Details are available here:
  // http://wiki.neillcorlett.com/PSFTagFormat
  size_t tagCurPos = PSF_TAG_SIG_LEN;
  while (tagCurPos < tagSectionSize) {
    // Search the end position of the current line.
    char *pNewLine = strchr(&tagSect[tagCurPos], 0x0a);
    if (pNewLine == nullptr) {
      // Tag section must end with a newline.
      // Read the all remaining bytes if a newline lacks though.
      pNewLine = tagSect + tagSectionSize;
    }

    // Replace the newline with NUL,
    // for better C string function compatibility.
    *pNewLine = '\0';

    // Search the variable=value separator.
    char *pSeparator = strchr(&tagSect[tagCurPos], '=');
    if (pSeparator == nullptr) {
      // Blank lines, or lines not of the form "variable=value", are ignored.
      tagCurPos = pNewLine + 1 - tagSect;
      continue;
    }

    // Determine the start/end position of variable.
    char *pVarName = &tagSect[tagCurPos];
    char *pVarNameEnd = pSeparator;
    char *pVarVal = pSeparator + 1;
    char *pVarValEnd = pNewLine;

    // Whitespace at the beginning/end of the line and before/after the = are ignored.
    // All characters 0x01-0x20 are considered whitespace.
    // (There must be no null (0x00) characters.)
    // Trim them.
    while (pVarNameEnd > pVarName && *reinterpret_cast<unsigned char *>(pVarNameEnd - 1) <= 0x20)
      pVarNameEnd--;
    while (pVarValEnd > pVarVal && *reinterpret_cast<unsigned char *>(pVarValEnd - 1) <= 0x20)
      pVarValEnd--;
    while (pVarName < pVarNameEnd && *reinterpret_cast<unsigned char *>(pVarName) <= 0x20)
      pVarName++;
    while (pVarVal < pVarValEnd && *reinterpret_cast<unsigned char *>(pVarVal) <= 0x20)
      pVarVal++;

    // Read variable=value as string.
    string varName(pVarName, pVarNameEnd - pVarName);
    string varVal(pVarVal, pVarValEnd - pVarVal);

    // Multiple-line variables must appear as consecutive lines using the same variable name.
    // For instance:
    //   comment=This is a
    //   comment=multiple-line
    //   comment=comment.
    // Therefore, check if the variable had already appeared.
    auto it = tags.lower_bound(varName);
    if (it != tags.end() && it->first == varName) {
      it->second += "\n";
      it->second += varVal;
    }
    else {
      tags.insert(it, make_pair(varName, varVal));
    }

    tagCurPos = pNewLine + 1 - tagSect;
  }
  delete[] tagSect;

  return true;
}

bool PSFFile::ReadExe(uint8_t *buf, size_t len, size_t stripLen) const {
  if (len == 0) {
    return true;
  }

  uLong destlen = static_cast<uLong>(len);
  int zRet = myuncompress(buf, &destlen, exeCompData->data, static_cast<uLong>(exeCompData->size), static_cast<uLong>(stripLen));
  if (zRet != Z_OK) {
    //errorstr = L"Decompression failed";
    return false;
  }
  return true;
}

bool PSFFile::ReadExeDataSeg(DataSeg *&seg, size_t len, size_t stripLen) const {
  DataSeg *newSeg = new DataSeg();
  if (len == 0) {
    seg = newSeg;
    return true;
  }

  uint8_t *buf = new uint8_t[len];
  uLong destlen = static_cast<uLong>(len);
  int zRet = myuncompress(buf, &destlen, exeCompData->data, static_cast<uLong>(exeCompData->size), static_cast<uLong>(stripLen));
  if (zRet != Z_OK) {
    //errorstr = L"Decompression failed";
    seg = nullptr;
    delete newSeg;
    delete[] buf;
    return false;
  }

  size_t actualSize = destlen;
  newSeg->load(buf, 0, actualSize);
  seg = newSeg;
  return true;
}

bool PSFFile::Decompress(size_t decompressed_size) {
  if (decompressed_size == 0) {
    exeData->clear();
    if (exeCompData->size == 0) {
      return true;
    }
    else {
      errorstr = L"Decompression failed";
      return false;
    }
  }

  uint8_t *buf = new uint8_t[decompressed_size];

  uLong destlen = static_cast<uLong>(decompressed_size);
  int zRet = uncompress(buf, &destlen, exeCompData->data, static_cast<uLong>(exeCompData->size));
  if (zRet != Z_STREAM_END) {
    errorstr = L"Decompression failed";
    delete[] buf;
    return false;
  }
  size_t actualSize = destlen;

  exeData->load(buf, 0, actualSize);
  decompressed = true;
  return true;
}

bool PSFFile::IsDecompressed() const {
  return decompressed;
}

uint8_t PSFFile::GetVersion() const {
  return version;
}

size_t PSFFile::GetExeSize() const {
  return exeData->size;
}

size_t PSFFile::GetCompressedExeSize() const {
  return exeCompData->size;
}

size_t PSFFile::GetReservedSize() const {
  return reservedData->size;
}

void PSFFile::Clear() {
  exeData->clear();
  exeCompData->clear();
  reservedData->clear();
  tags.clear();
  version = 0;
  decompressed = false;
  errorstr = nullptr;
  parent = nullptr;
}

const wchar_t *PSFFile::GetError() const {
  return errorstr;
}

// original from zlib/uncompr.c
int PSFFile::myuncompress(
    Bytef *dest,
    uLongf *destLen,
    z_const Bytef *source,
    uLong sourceLen,
    uLong stripLen) const {
  z_stream stream;
  int err;

  uLong stripAvailLen = 0;
  uLong strippedLen = 0;

  stream.next_in = static_cast<z_const Bytef *>(source);
  stream.avail_in = static_cast<uInt>(sourceLen);
  /* Check for source > 64K on 16-bit machine: */
  if (static_cast<uLong>(stream.avail_in) != sourceLen) return Z_BUF_ERROR;

  stream.next_out = dest;
  stream.avail_out = static_cast<uInt>(*destLen);
  if (static_cast<uLong>(stream.avail_out) != *destLen) return Z_BUF_ERROR;

  if (stripLen != 0) {
    stripAvailLen = min(stripLen, static_cast<uLong>(stripBufSize));
    stream.next_out = static_cast<z_const Bytef *>(stripBuf);
    stream.avail_out = static_cast<uInt>(stripAvailLen);
  }

  stream.zalloc = nullptr;
  stream.zfree = nullptr;

  err = inflateInit_((&stream), ZLIB_VERSION, sizeof(z_stream));
  if (err != Z_OK) return err;

  while (true) {
    err = inflate(&stream, Z_NO_FLUSH);
    if (err == Z_OK && strippedLen + stripAvailLen < stripLen) {
      // try stripping more bytes
      strippedLen += stripAvailLen;
      stripAvailLen = min(stripLen - strippedLen, static_cast<uLong>(stripBufSize));
      stream.next_out = static_cast<z_const Bytef *>(stripBuf);
      stream.avail_out = static_cast<uInt>(stripAvailLen);
    }
    else if (err == Z_OK && stripAvailLen != 0) {
      // finish stripping
      strippedLen += stripAvailLen;
      stripAvailLen = 0;
      stream.next_out = dest;
      stream.avail_out = static_cast<uInt>(*destLen);
    }
    else {
      if (err == Z_OK) {
        inflateEnd(&stream);
        if (stripAvailLen != 0)
          *destLen = 0;
        else
          *destLen = stream.total_out;
        return Z_OK;
      }
      else if (err != Z_STREAM_END) {
        inflateEnd(&stream);
        if (err == Z_NEED_DICT)
          return Z_DATA_ERROR;
        return err;
      }
      *destLen = stream.total_out;
      break;
    }
  }

  err = inflateEnd(&stream);
  return err;
}
