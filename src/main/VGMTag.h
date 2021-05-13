#pragma once

class VGMTag {
 public:
  VGMTag();
  VGMTag(const std::wstring &_title, const std::wstring &_artist = L"", const std::wstring &_album = L"");
  virtual ~VGMTag();

  bool HasTitle() const;
  bool HasArtist() const;
  bool HasAlbum() const;
  bool HasComment() const;
  bool HasTrackNumber() const;
  bool HasLength() const;

 public:
  std::wstring title;
  std::wstring artist;
  std::wstring album;
  std::wstring comment;
  std::map<std::wstring, std::vector<uint8_t> > binaries;

  /** Track number */
  int track_number;

  /** Length in seconds */
  double length;
};
