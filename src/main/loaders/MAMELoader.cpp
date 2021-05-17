#include "pch.h"
#include <tinyxml.h>
#include "MAMELoader.h"
#include "Root.h"
#include "Format.h"
#include "KabukiDecrypt.h"

using namespace std;

//template < class T >
//bool MAMERomGroupEntry::GetAttribute( const std::string& attrName, T* out )
//{
//	string strValue = attributes[attrName];
//	if (strValue.empty())
//		return false;			//Attribute name does not exist.
//
//	FromString(strValue, out);
//	return true;
//}

bool MAMERomGroupEntry::GetHexAttribute(const std::string &attrName, uint32_t *out) {
  string strValue = attributes[attrName];
  if (strValue.empty())
    return false;            //Attribute name does not exist.

  *out = StringToHex(strValue);
  return true;
}

MAMERomGroupEntry *MAMEGameEntry::GetRomGroupOfType(const string &strType) {
  for (auto &romgroup: romgroupentries) {
    if (romgroup.type == strType)
      return &romgroup;
  }
  return nullptr;
}


MAMELoader::MAMELoader() {
  bLoadedXml = !LoadXML();
}

MAMELoader::~MAMELoader() {
  DeleteMap<string, MAMEGameEntry>(gamemap);
}

int MAMELoader::LoadXML() {
  TiXmlDocument doc("mame_roms.xml");
  if (!doc.LoadFile())        //if loading the xml file fails
  {
    return 1;
  }
  TiXmlElement *rootElmt = doc.FirstChildElement();
  const string &className = rootElmt->ValueStr();
  if (className != "romlist")
    return 1;

  /// for all "game" elements
  for (TiXmlElement *gameElmt = rootElmt->FirstChildElement(); gameElmt != 0;
       gameElmt = gameElmt->NextSiblingElement()) {
    if (gameElmt->ValueStr() != "game")
      return 1;
    MAMEGameEntry *gameentry = LoadGameEntry(gameElmt);
    if (!gameentry)
      return 1;
    gamemap[gameentry->name] = gameentry;
  }
  return 0;
}

MAMEGameEntry *MAMELoader::LoadGameEntry(TiXmlElement *gameElmt) {
  MAMEGameEntry *gameentry = new MAMEGameEntry;
  string gamename, fmtVersionStr;

  if (gameElmt->QueryValueAttribute("name", &gameentry->name) != TIXML_SUCCESS) {
    delete gameentry;
    return nullptr;
  }
  if (gameElmt->QueryValueAttribute("format", &gameentry->format) != TIXML_SUCCESS) {
    delete gameentry;
    return nullptr;
  }
  if (gameElmt->QueryValueAttribute("fmt_version", &fmtVersionStr) != TIXML_SUCCESS) {
    gameentry->fmt_version = 0;
    gameentry->fmt_version_str = "";
  }
  else {
    FromString(fmtVersionStr, &gameentry->fmt_version);
    gameentry->fmt_version_str = fmtVersionStr;
    //stringstream convert ( fmtVersionStr );
    //convert >> std::setprecision(2) >> gameentry->fmt_version;		//read seq_table as hexadecimal value
  }

  // Load rom groups
  for (TiXmlElement *romgroupElmt = gameElmt->FirstChildElement(); romgroupElmt != 0;
       romgroupElmt = romgroupElmt->NextSiblingElement()) {
    if (romgroupElmt->ValueStr() != "romgroup") {
      delete gameentry;
      return nullptr;
    }
    if (LoadRomGroupEntry(romgroupElmt, gameentry)) {
      delete gameentry;
      return nullptr;
    }
  }
  return gameentry;
}

int MAMELoader::LoadRomGroupEntry(TiXmlElement *romgroupElmt, MAMEGameEntry *gameentry) {
  MAMERomGroupEntry romgroupentry;

  //First, get the "type" and "load_method" attributes.  If they don't exist, we return with an error.
  string load_method;
  if (romgroupElmt->QueryValueAttribute("type", &romgroupentry.type) != TIXML_SUCCESS)
    return 1;
  if (romgroupElmt->QueryValueAttribute("load_method", &load_method) != TIXML_SUCCESS)
    return 1;

  // Read the encryption type string, if it exists
  romgroupElmt->QueryValueAttribute("encryption", &romgroupentry.encryption);

  //Iterate through the attributes of the romgroup element, recording any user-defined values.
  for (TiXmlAttribute *attr = romgroupElmt->FirstAttribute(); attr; attr = attr->Next()) {
    // NameTStr() is returning an std::string because we have defined TIXML_USE_STL
    const string &attrName = attr->NameTStr();
    // Ignore the attribute if it is "type" or "load_method"; we already dealt with those, they are mandated.
    if (attrName == "type" && attrName =="load_method")
      romgroupentry.attributes[attrName] = attr->ValueStr();
  }

  if (load_method == "append")
    romgroupentry.loadmethod = LM_APPEND;
  else if (load_method == "append_swap16")
    romgroupentry.loadmethod = LM_APPEND_SWAP16;
  else if (load_method == "deinterlace")
    romgroupentry.loadmethod = LM_DEINTERLACE;
  else
    return 1;

  // load rom entries
  for (TiXmlElement *romElmt = romgroupElmt->FirstChildElement(); romElmt != 0;
       romElmt = romElmt->NextSiblingElement()) {
    const string &elmtName = romElmt->ValueStr();
    if (elmtName != "rom")
      return 1;
    romgroupentry.roms.emplace_back(romElmt->GetText());
  }

  gameentry->romgroupentries.push_back(romgroupentry);
  return 0;
}


PostLoadCommand MAMELoader::Apply(RawFile *file) {
  if (!bLoadedXml)
    return KEEP_IT;
  if (file->GetExtension() != L"zip")
    return KEEP_IT;

  wstring fullfilename_w = file->GetFileName();
  string fullfilename = wstring2string(fullfilename_w);
  const char *endoffilename = strrchr(fullfilename.c_str(), '.');
  char filename[10] = {0};
  strncpy(filename, fullfilename.c_str(), endoffilename - fullfilename.c_str());

  // look up the game name in our little database
  auto it = gamemap.find(filename);
  if (it == gamemap.end())        //if we couldn't find an entry for the game name
    return KEEP_IT;               //don't do anything

  MAMEGameEntry *gameentry = it->second;

  //Get the format given and check if it is defined in VGMTrans
  Format *fmt = Format::GetFormatFromName(gameentry->format);
  if (!fmt)
    return KEEP_IT;

  //try to open up the game zip
  wstring fullpath = file->GetFullPath();
  string test = wstring2string(fullpath);
  unzFile cur_file = unzOpen(wstring2string(fullpath).c_str());
  if (!cur_file) {
    return KEEP_IT;
  }
  int ret;

  //Now we try to load the rom groups.  We save the created file into the rom MAMERomGroupEntry's file member
  // Note that this does not check for an error, so the romgroup entry's file member may receive nullptr.
  // This must be checked for in Scan().
  for (auto &romgroup : gameentry->romgroupentries) {
    romgroup.file = LoadRomGroup(&romgroup, gameentry->format, cur_file);
  }

  fmt->GetScanner().Scan(nullptr, gameentry);

  for (auto &romgroup : gameentry->romgroupentries) {
    if (romgroup.file != nullptr)
      pRoot->SetupNewRawFile(romgroup.file);
  }

  ret = unzClose(cur_file);
  if (ret != UNZ_OK) {
    return KEEP_IT;
  }

  return DELETE_IT;
}


VirtFile *MAMELoader::LoadRomGroup(MAMERomGroupEntry *entry, const string &format, unzFile &cur_file) {
  uint32_t destFileSize = 0;
  list<pair<uint8_t *, uint32_t>> buffers;
  list<string> &roms = entry->roms;
  for (auto & rom : roms) {
    long ret = unzLocateFile(cur_file, rom.c_str(), 0);
    if (ret == UNZ_END_OF_LIST_OF_FILE) {
      //file not found
      DeleteBuffers(buffers);
      return nullptr;
    }
    unz_file_info info;
    ret = unzGetCurrentFileInfo(cur_file, &info, 0, 0, 0, 0, 0, 0);
    if (ret != UNZ_OK) {
      //could not get zipped file info
      DeleteBuffers(buffers);
      return nullptr;
    }
    destFileSize += info.uncompressed_size;
    ret = unzOpenCurrentFile(cur_file);
    if (ret != UNZ_OK) {
      //could not open file in zip archive
      DeleteBuffers(buffers);
      return nullptr;
    }
    uint8_t *buf = new uint8_t[info.uncompressed_size];
    ret = unzReadCurrentFile(cur_file, buf, info.uncompressed_size);
    if (ret != static_cast<long>(info.uncompressed_size)) {
      //error reading file in zip archive
      delete[] buf;
      DeleteBuffers(buffers);
      return nullptr;
    }
    ret = unzCloseCurrentFile(cur_file);
    if (ret != UNZ_OK) {
      //could not close file in zip archive
      DeleteBuffers(buffers);
      return nullptr;
    }
    buffers.emplace_back(buf, info.uncompressed_size);
  }

  uint8_t *destFile = new uint8_t[destFileSize];
  switch (entry->loadmethod) {

    // append the files
    case LM_APPEND: {
      uint32_t curOffset = 0;
      for (auto & buffer : buffers) {
        memcpy(destFile + curOffset, buffer.first, buffer.second);
        curOffset += buffer.second;
      }
      break;
    }

    // append the files and swap every 16 byte word
    case LM_APPEND_SWAP16: {
      uint32_t curDestOffset = 0;
      for (auto & buffer : buffers) {

        uint8_t *romBuf = buffer.first;
        uint32_t romSize = buffer.second;
        for (uint32_t i = 0; i < romSize; i += 2) {
          destFile[curDestOffset + i] = romBuf[i + 1];
          destFile[curDestOffset + i + 1] = romBuf[i];
        }
        curDestOffset += buffer.second;
      }
      break;
    }

    // interlace the bytes from each rom
    case LM_DEINTERLACE: {
      uint32_t curDestOffset = 0;
      uint32_t curRomOffset = 0;
      while (curDestOffset < destFileSize) {
        for (auto & buffer : buffers)
          destFile[curDestOffset++] = buffer.first[curRomOffset];
        curRomOffset++;
      }
      break;
    }
  }
  DeleteBuffers(buffers);

  // If an encryption type is defined, decrypt the data
  if (!entry->encryption.empty()) {
    if (entry->encryption == "kabuki") {
      uint32_t swap_key1, swap_key2, addr_key, xor_key;
      if (!entry->GetHexAttribute("kabuki_swap_key1", &swap_key1) ||
          !entry->GetHexAttribute("kabuki_swap_key2", &swap_key2) ||
          !entry->GetHexAttribute("kabuki_addr_key", &addr_key) ||
          !entry->GetHexAttribute("kabuki_xor_key", &xor_key)) {
        delete[] destFile;
        return nullptr;
      }
      uint8_t *decrypt = new uint8_t[0x8000];
      KabukiDecrypter::kabuki_decode(destFile,
                                     decrypt,
                                     destFile,
                                     0x0000,
                                     0x8000,
                                     swap_key1,
                                     swap_key2,
                                     addr_key,
                                     xor_key);
      //pRoot->UI_WriteBufferToFile(L"opcodesdump", decrypt, destFileSize);
      delete[] decrypt;
    }
  }

  //static int num = 0;
  //wostringstream	fn;
  //fn << L"romgroup " << num++;
  //pRoot->UI_WriteBufferToFile(fn.str().c_str(), destFile, destFileSize);
  wostringstream strstream;
  strstream << L"romgroup  - " << entry->type.c_str();
  VirtFile *newVirtFile = new VirtFile(destFile, destFileSize, strstream.str());
  newVirtFile->DontUseLoaders();
  newVirtFile->DontUseScanners();
  newVirtFile->SetProPreRatio(0.80);
  return newVirtFile;
}


void MAMELoader::DeleteBuffers(list<pair<uint8_t *, uint32_t>> &buffers) {
  for (auto & buffer : buffers)
    delete[] buffer.first;
}