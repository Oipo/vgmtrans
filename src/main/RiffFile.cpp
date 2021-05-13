#include "pch.h"
#include "RiffFile.h"

#include <utility>

using namespace std;

uint32_t Chunk::GetSize() {
  return 8 + GetPaddedSize(size);
}

void Chunk::SetData(const void *src, uint32_t datasize) {
  size = datasize;

  // set the size and copy from the data source
  datasize = GetPaddedSize(size);
  if (data != nullptr) {
    delete[] data;
    data = nullptr;
  }
  data = new uint8_t[datasize];
  memcpy(data, src, size);

  // Add pad byte
  uint32_t padsize = datasize - size;
  if (padsize != 0) {
    memset(data + size, 0, padsize);
  }
}

void Chunk::Write(uint8_t *buffer) {
  uint32_t padsize = GetPaddedSize(size) - size;
  memcpy(buffer, id, 4);
  *reinterpret_cast<uint32_t *>(buffer + 4) = size + padsize; // Microsoft says the chunkSize doesn't contain padding size, but many software cannot handle the alignment.
  memcpy(buffer + 8, data, GetPaddedSize(size));
}

Chunk *ListTypeChunk::AddChildChunk(Chunk *ck) {
  childChunks.push_back(ck);
  return ck;
}

uint32_t ListTypeChunk::GetSize() {
  uint32_t _size = 12;        //id + size + "LIST"
  for(auto &child : childChunks) {
    _size += child->GetSize();
  }
  return GetPaddedSize(_size);
}

void ListTypeChunk::Write(uint8_t *buffer) {
  memcpy(buffer, this->id, 4);
  memcpy(buffer + 8, this->type, 4);

  uint32_t bufOffset = 12;
  for(auto &child : childChunks) {
    child->Write(buffer + bufOffset);
    bufOffset += child->GetSize();
  }

  uint32_t _size = bufOffset;
  uint32_t padsize = GetPaddedSize(_size) - _size;
  *reinterpret_cast<uint32_t *> (buffer + 4) = _size + padsize - 8; // Microsoft says the chunkSize doesn't contain padding size, but many software cannot handle the alignment.

  // Add pad byte
  if (padsize != 0) {
    memset(data + _size, 0, padsize);
  }
}

RiffFile::RiffFile(string file_name, string form)
    : RIFFChunk(std::move(form)),
      name(std::move(file_name)) {
}
