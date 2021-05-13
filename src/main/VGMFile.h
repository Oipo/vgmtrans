#pragma once
#include "VGMItem.h"
#include "DataSeg.h"
#include "RawFile.h"
#include "Menu.h"
#include <optional>

enum FmtID: unsigned int;
class VGMColl;
class Format;

enum FileType { FILETYPE_UNDEFINED, FILETYPE_SEQ, FILETYPE_INSTRSET, FILETYPE_SAMPCOLL, FILETYPE_MISC };


// MACROS

/*#define USING_FORMAT(fmt_id)									\
	public:														\
	virtual Format* GetFormat()									\
	{															\
		return pRoot->GetFormat(fmt_id);						\
	}
*/



class VGMFile:
    public VGMContainerItem {
 public:
  BEGIN_MENU(VGMFile)
      MENU_ITEM(VGMFile, OnClose, L"Close")
      MENU_ITEM(VGMFile, OnSaveAsRaw, L"Save as original format")
      //MENU_ITEM(VGMFile, OnSaveAllAsRaw, L"Save all as original format")
  END_MENU()

 public:
  VGMFile(FileType fileType, /*FmtID fmtID,*/
          const std::string &format,
          RawFile *theRawFile,
          uint32_t offset,
          uint32_t length = 0,
          std::wstring theName = L"VGM File");
  ~VGMFile() override;

  [[nodiscard]] ItemType GetType() const override { return ITEMTYPE_VGMFILE; }
  FileType GetFileType() { return file_type; }

  void AddToUI(VGMItem *parent, void *UI_specific) override;

  [[nodiscard]] const std::wstring *GetName() const;

  bool OnClose();
  bool OnSaveAsRaw();
  bool OnSaveAllAsRaw();

  bool LoadVGMFile();
  virtual bool Load() = 0;
  Format *GetFormat();
  const std::string &GetFormatName();

  virtual std::optional<uint32_t> GetID() { return id; }

  void AddCollAssoc(VGMColl *coll);
  void RemoveCollAssoc(VGMColl *coll);
  [[nodiscard]] RawFile *GetRawFile() const override;
  void LoadLocalData();
  void UseLocalData();
  void UseRawFileData();

 public:
  uint32_t GetBytes(uint32_t nIndex, uint32_t nCount, void *pBuffer) override;

  uint8_t GetByte(uint32_t offset) override {
    if (bUsingRawFile)
      return rawfile->GetByte(offset);
    else
      return data[offset];
  }

  uint16_t GetShort(uint32_t offset) override {
    if (bUsingRawFile)
      return rawfile->GetShort(offset);
    else
      return data.GetShort(offset);
  }

  uint32_t GetWord(uint32_t offset) override {
    if (bUsingRawFile)
      return rawfile->GetWord(offset);
    else
      return data.GetWord(offset);
  }

  //GetShort Big Endian
  uint16_t GetShortBE(uint32_t offset) override {
    if (bUsingRawFile)
      return rawfile->GetShortBE(offset);
    else
      return data.GetShortBE(offset);
  }

  //GetWord Big Endian
  uint32_t GetWordBE(uint32_t offset) override {
    if (bUsingRawFile)
      return rawfile->GetWordBE(offset);
    else
      return data.GetWordBE(offset);
  }

  bool IsValidOffset(uint32_t offset) override {
    if (bUsingRawFile)
      return rawfile->IsValidOffset(offset);
    else
      return data.IsValidOffset(offset);
  }

  [[nodiscard]] uint32_t GetStartOffset() const {
    if (bUsingRawFile)
      return 0;
    else
      return data.startOff;
  }

  [[nodiscard]] inline uint32_t GetEndOffset() const {
    if (bUsingRawFile)
      return rawfile->size();
    else
      return data.endOff;
  }

  [[nodiscard]] inline unsigned long size() const {
    if (bUsingRawFile)
      return rawfile->size();
    else
      return data.size;
  }

 protected:
  DataSeg data, col;
  FileType file_type;
  const std::string &format;
  std::optional<uint32_t> id{};
  std::wstring name;
 public:
  RawFile *rawfile;
  std::list<VGMColl *> assocColls;
  bool bUsingRawFile;
  bool bUsingCompressedLocalData;
};


// *********
// VGMHeader
// *********

class VGMHeader:
    public VGMContainerItem {
 public:
  VGMHeader(VGMItem *parItem, uint32_t offset = 0, uint32_t length = 0, const std::wstring &name = L"Header");
  ~VGMHeader() override;

  Icon GetIcon() override { return ICON_BINARY; };

  void AddPointer(uint32_t offset, uint32_t length, uint32_t destAddress, bool notNull, const std::wstring &name = L"Pointer");
  void AddTempo(uint32_t offset, uint32_t length, const std::wstring &name = L"Tempo");
  void AddSig(uint32_t offset, uint32_t length, const std::wstring &name = L"Signature");

  //vector<VGMItem*> items;
};

// *************
// VGMHeaderItem
// *************

class VGMHeaderItem:
    public VGMItem {
 public:
  enum HdrItemType { HIT_POINTER, HIT_TEMPO, HIT_SIG, HIT_GENERIC, HIT_UNKNOWN };        //HIT = Header Item Type

  VGMHeaderItem(VGMHeader *hdr, HdrItemType theType, uint32_t offset, uint32_t length, const std::wstring &name);
  Icon GetIcon() override;

 public:
  HdrItemType type;
};
