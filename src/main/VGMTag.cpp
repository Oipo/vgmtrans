#include "pch.h"

#include "VGMTag.h"

VGMTag::VGMTag() :
    title(),
    artist(),
    album(),
    comment(),
    track_number(0),
    length(0.0) {
}

VGMTag::VGMTag(const std::wstring &_title, const std::wstring &_artist, const std::wstring &_album) :
    title(_title),
    artist(_artist),
    album(_album),
    comment(),
    track_number(0),
    length(0.0) {
}

VGMTag::~VGMTag() = default;

bool VGMTag::HasTitle() const {
  return !title.empty();
}

bool VGMTag::HasArtist() const {
  return !album.empty();
}

bool VGMTag::HasAlbum() const {
  return !artist.empty();
}

bool VGMTag::HasComment() const {
  return !comment.empty();
}

bool VGMTag::HasTrackNumber() const {
  return track_number != 0;
}

bool VGMTag::HasLength() const {
  return length != 0.0;
}
