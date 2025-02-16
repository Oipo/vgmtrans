#include "pch.h"
#include "ExtensionDiscriminator.h"

//static ExtensionDiscriminator theExtensionDiscriminator;
//ExtensionDiscriminator ExtensionDiscriminator::instance;

ExtensionDiscriminator::ExtensionDiscriminator() {
}

ExtensionDiscriminator::~ExtensionDiscriminator() {
}


int ExtensionDiscriminator::AddExtensionScannerAssoc(std::wstring extension, VGMScanner *scanner) {
  mScannerExt[extension].push_back(scanner);
  return true;
}

std::list<VGMScanner *> *ExtensionDiscriminator::GetScannerList(std::wstring extension) {
  std::map<std::wstring, std::list<VGMScanner *> >::iterator iter = mScannerExt.find(StringToLower(extension));
  if (iter == mScannerExt.end())
    return nullptr;
  else
    return &(*iter).second;
}
