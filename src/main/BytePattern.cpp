// Byte pattern class for flexible byte sequence search
// Heavily inspired by SigScan at GameDeception.net

#include "pch.h"

#include <assert.h>
#include "BytePattern.h"

BytePattern::BytePattern() :
    ptn_str(nullptr),
    ptn_mask(nullptr),
    ptn_len(0) {
}

BytePattern::BytePattern(const char *pattern, size_t length) :
    ptn_str(nullptr),
    ptn_mask(nullptr),
    ptn_len(length) {
  if (length != 0) {
    if (pattern != nullptr) {
      ptn_str = new char[length];
      memcpy(ptn_str, pattern, length);
    }
  }
}

BytePattern::BytePattern(const char *pattern, const char *mask, size_t length) :
    ptn_str(nullptr),
    ptn_mask(nullptr),
    ptn_len(length) {
  if (length != 0) {
    if (pattern != nullptr) {
      ptn_str = new char[length];
      memcpy(ptn_str, pattern, length);

      if (mask != nullptr) {
        assert(strlen(mask) == length);

        ptn_mask = new char[length];
        memcpy(ptn_mask, mask, length);
      }
    }
  }
}

BytePattern::BytePattern(const BytePattern &obj) :
    ptn_str(nullptr),
    ptn_mask(nullptr),
    ptn_len(obj.ptn_len) {
  if (obj.ptn_str != nullptr) {
    ptn_str = new char[ptn_len];
    memcpy(ptn_str, obj.ptn_str, ptn_len);
  }

  if (obj.ptn_mask != nullptr) {
    ptn_mask = new char[ptn_len];
    memcpy(ptn_mask, obj.ptn_mask, ptn_len);
  }
}

BytePattern::~BytePattern() {
  if (ptn_str != nullptr) {
    delete[] ptn_str;
  }

  if (ptn_mask != nullptr) {
    delete[] ptn_mask;
  }
}

bool BytePattern::match(const void *buf, size_t buf_len) const {
  if (buf == nullptr)
    return false;

  if (ptn_str == nullptr)
    return false;

  if (ptn_len > buf_len)
    return false;

  for (size_t i = 0; i < ptn_len; i++) {
    // '?' matches to any characters
    if (ptn_mask == nullptr || ptn_mask[i] != '?') {
      if (static_cast<const char *>(buf)[i] != ptn_str[i]) {
        return false;
      }
    }
  }
  return true;
}

bool BytePattern::search(const void *buf, size_t buf_len, size_t &match_offset, size_t search_offset) const {
  if (buf == nullptr)
    return false;

  if (ptn_str == nullptr)
    return false;

  if (search_offset + ptn_len > buf_len)
    return false;

  for (size_t i = search_offset; i <= buf_len - ptn_len; i++) {
    if (match(static_cast<const char *>(buf) + i, ptn_len)) {
      match_offset = i;
      return true;
    }
  }
  return false;
}
