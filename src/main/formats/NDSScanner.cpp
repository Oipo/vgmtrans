#include "pch.h"
#include "common.h"
#include "NDSScanner.h"
#include "NDSSeq.h"
#include "NDSInstrSet.h"

using namespace std;

#define SRCH_BUF_SIZE 0x20000


void NDSScanner::Scan(RawFile *file, void *info) {
  SearchForSDAT(file);
}

void NDSScanner::SearchForSDAT(RawFile *file) {
  uint32_t nFileLength = file->size();
  for (uint32_t i = 0; i + 4 < nFileLength; i++) {
    if ((*file)[i] == 'S' && (*file)[i + 1] == 'D' && (*file)[i + 2] == 'A' && (*file)[i + 3] == 'T'
        && (*file)[i + 4] == 0xFF && (*file)[i + 5] == 0xFE && (*file)[i + 6] == 0
        && (*file)[i + 7] == 0x01 && (file->GetShort(i + 12) < 0x100)
        && (file->GetWord(i + 0x10) < 0x10000)) {
      i += LoadFromSDAT(file, i);
    }
  }
}


// The following is pretty god-awful messy.  I should have created structs for the different
// blocks and loading the entire blocks at a time.  
uint32_t NDSScanner::LoadFromSDAT(RawFile *file, uint32_t baseOff) {
  uint32_t SYMBoff;
  uint32_t INFOoff;
  uint32_t FAToff;
  uint32_t nSeqs;
  uint32_t nBnks;
  uint32_t nWAs;
  vector<wstring> seqNames;
  vector<wstring> bnkNames;
  vector<wstring> waNames;
  vector<uint16_t> seqFileIDs;
  vector<uint16_t> bnkFileIDs;
  vector<uint16_t> waFileIDs;
  vector<uint16_t> seqFileBnks;
  vector<vector<uint16_t> > bnkWAs;
  vector<NDSWaveArch *> WAs;
  vector<pair<uint16_t, NDSInstrSet *> > BNKs;

  uint32_t SDATLength = file->GetWord(baseOff + 8) + 8;

  SYMBoff = file->GetWord(baseOff + 0x10) + baseOff;
  INFOoff = file->GetWord(baseOff + 0x18) + baseOff;
  FAToff = file->GetWord(baseOff + 0x20) + baseOff;
  bool hasSYMB = (SYMBoff != baseOff);

  nSeqs = file->GetWord(file->GetWord(INFOoff + 0x08) + INFOoff);
  nBnks = file->GetWord(file->GetWord(INFOoff + 0x10) + INFOoff);
  nWAs = file->GetWord(file->GetWord(INFOoff + 0x14) + INFOoff);

  uint32_t pSeqNamePtrList;
  uint32_t pBnkNamePtrList;
  uint32_t pWANamePtrList;
  if (hasSYMB) {
    pSeqNamePtrList = file->GetWord(SYMBoff + 0x08) + SYMBoff;        //get pointer to list of sequence name pointers
    pBnkNamePtrList = file->GetWord(SYMBoff + 0x10) + SYMBoff;        //get pointer to list of bank name pointers
    pWANamePtrList = file->GetWord(SYMBoff + 0x14) + SYMBoff;        //get pointer to list of wavearchive name pointers
  }

  for (uint32_t i = 0; i < nSeqs; i++) {
    char temp[32];        //that 32 is totally arbitrary, i should change it
    wchar_t wtemp[32];
    if (hasSYMB) {
      file->GetBytes(file->GetWord(pSeqNamePtrList + 4 + i * 4) + SYMBoff, 32, temp);
      mbstowcs(wtemp, temp, 32);
    }
    else {
      swprintf(wtemp, 32, L"SSEQ_%04d", i);
    }
    seqNames.emplace_back(wtemp);
  }

  for (uint32_t i = 0; i < nBnks; i++) {
    char temp[32];        //that 32 is totally arbitrary, i should change it
    wchar_t wtemp[32];

    if (hasSYMB) {
      file->GetBytes(file->GetWord(pBnkNamePtrList + 4 + i * 4) + SYMBoff, 32, temp);
      mbstowcs(wtemp, temp, 32);
    }
    else {
      swprintf(wtemp, 32, L"SBNK_%04d", i);
    }
    bnkNames.emplace_back(wtemp);
  }

  for (uint32_t i = 0; i < nWAs; i++) {
    char temp[32];        //that 32 is totally arbitrary, i should change it
    wchar_t wtemp[32];

    if (hasSYMB) {
      file->GetBytes(file->GetWord(pWANamePtrList + 4 + i * 4) + SYMBoff, 32, temp);
      mbstowcs(wtemp, temp, 32);
    }
    else {
      swprintf(wtemp, 32, L"SWAR_%04d", i);
    }
    waNames.emplace_back(wtemp);
  }

  uint32_t pSeqInfoPtrList = file->GetWord(INFOoff + 8) + INFOoff;
  //uint32_t seqInfoPtrListLength = file->GetWord(INFOoff + 12);
  uint32_t nSeqInfos = file->GetWord(pSeqInfoPtrList);
  for (uint32_t i = 0; i < nSeqInfos; i++) {
    uint32_t pSeqInfoUnadjusted = file->GetWord(pSeqInfoPtrList + 4 + i * 4);
    uint32_t pSeqInfo = INFOoff + pSeqInfoUnadjusted;
    if (pSeqInfoUnadjusted == 0)
      seqFileIDs.push_back(std::numeric_limits<uint16_t>::max());
    else
      seqFileIDs.push_back(file->GetShort(pSeqInfo));
    seqFileBnks.push_back(file->GetShort(pSeqInfo + 4));
    //next bytes would be vol, cpr, ppr, and ply respectively, whatever the heck those last 3 stand for
  }

  uint32_t pBnkInfoPtrList = file->GetWord(INFOoff + 0x10) + INFOoff;
  uint32_t nBnkInfos = file->GetWord(pBnkInfoPtrList);
  for (uint32_t i = 0; i < nBnkInfos; i++) {
    uint32_t pBnkInfoUnadjusted = file->GetWord(pBnkInfoPtrList + 4 + i * 4);
    uint32_t pBnkInfo = INFOoff + pBnkInfoUnadjusted;
    if (pBnkInfoUnadjusted == 0)
      bnkFileIDs.push_back(std::numeric_limits<uint16_t>::max());
    else
      bnkFileIDs.push_back(file->GetShort(pBnkInfo));
    bnkWAs.emplace_back();
    vector<vector<uint16_t> >::reference ref = bnkWAs.back();
    for (int j = 0; j < 4; j++) {
      uint16_t WANum = file->GetShort(pBnkInfo + 4 + (j * 2));
      //if (WANum > 0x200)			//insanity check
      if (WANum >= nWAs)
        ref.push_back(0xFFFF);
      else
        ref.push_back(WANum);
    }
  }

  uint32_t pWAInfoList = file->GetWord(INFOoff + 0x14) + INFOoff;
  uint32_t nWAInfos = file->GetWord(pWAInfoList);
  for (uint32_t i = 0; i < nWAInfos; i++) {
    uint32_t pWAInfoUnadjusted = file->GetWord(pWAInfoList + 4 + i * 4);
    uint32_t pWAInfo = INFOoff + pWAInfoUnadjusted;
    if (pWAInfoUnadjusted == 0)
      waFileIDs.push_back(std::numeric_limits<uint16_t>::max());
    else
      waFileIDs.push_back(file->GetShort(pWAInfo));
  }

  {
    vector<uint16_t> vUniqueWAs;// = vector<uint16_t>(bnkWAs);
    for (auto & bnkWA : bnkWAs)
      vUniqueWAs.insert(vUniqueWAs.end(), bnkWA.begin(), bnkWA.end());
    sort(vUniqueWAs.begin(), vUniqueWAs.end());
    auto new_end = unique(vUniqueWAs.begin(), vUniqueWAs.end());

    vector<bool> valid;
    valid.resize(nWAs);
    for (auto iter = vUniqueWAs.begin(); iter != new_end; iter++) {
      if ((*iter != std::numeric_limits<uint16_t>::max()) && (*iter < valid.size()))
        valid[*iter] = true;
    }

    for (uint32_t i = 0; i < nWAs; i++)
      //for (vector<uint16_t>::iterator iter = vUniqueWAs.begin(); iter != new_end; iter++)
    {
      if (valid[i] != 1 || waFileIDs[i] == std::numeric_limits<uint16_t>::max()) {
        WAs.push_back(nullptr);
        continue;
      }
      uint32_t offset = FAToff + 12 + waFileIDs[i] * 0x10;
      uint32_t pWAFatData = file->GetWord(offset) + baseOff;
      offset += 4;
      uint32_t fileSize = file->GetWord(offset);
      NDSWaveArch *NewNDSwa = new NDSWaveArch(file, pWAFatData, fileSize, waNames[i]);
      if (!NewNDSwa->LoadVGMFile()) {
        pRoot->AddLogItem(new LogItem(FormatString<wstring>(L"Failed to load NDSWaveArch at 0x%08x\n",
                                                            pWAFatData).c_str(), LOG_LEVEL_ERR, L"NDSScanner"));
        WAs.push_back(nullptr);
        delete NewNDSwa;
        continue;
      }
      WAs.push_back(NewNDSwa);
    }
  }

  {
    vector<uint16_t> vUniqueBanks = vector<uint16_t>(seqFileBnks);
    sort(vUniqueBanks.begin(), vUniqueBanks.end());
    auto new_end = unique(vUniqueBanks.begin(), vUniqueBanks.end());

    //for (uint32_t i=0; i<nBnks; i++)
    //for (uint32_t i=0; i<seqFileBnks.size(); i++)
    for (auto iter = vUniqueBanks.begin(); iter != new_end; iter++) {
      if (*iter >= bnkFileIDs.size() /*0x1000*/|| bnkFileIDs[*iter] == std::numeric_limits<uint16_t>::max())    // > 0x1000 is idiot test for Phoenix Wright, which had many 0x1C80 values, as if they were 0xFFFF
        continue;
      uint32_t offset = FAToff + 12 + bnkFileIDs[*iter] * 0x10;
      uint32_t pBnkFatData = file->GetWord(offset) + baseOff;
      offset += 4;
      uint32_t fileSize = file->GetWord(offset);
      //if (bnkWAs[*iter][0] == std::numeric_limits<uint16_t>::max() || numWAs != 1)
      //	continue;
      NDSInstrSet *NewNDSInstrSet = new NDSInstrSet(file, pBnkFatData, fileSize,
                                                    bnkNames[*iter]/*, WAs[bnkWAs[*iter][0]]*/);
      for (int i = 0; i < 4; i++)        //use first WA found.  Ideally, should load all WAs
      {
        short WAnum = bnkWAs[*iter][i];
        if (WAnum != -1)
          NewNDSInstrSet->sampCollWAList.push_back(WAs[WAnum]);
        else
          NewNDSInstrSet->sampCollWAList.push_back(nullptr);
      }
      if (!NewNDSInstrSet->LoadVGMFile()) {
        pRoot->AddLogItem(new LogItem(FormatString<wstring>(L"Failed to load NDSInstrSet at 0x%08x\n",
                                                            pBnkFatData).c_str(), LOG_LEVEL_ERR, L"NDSScanner"));
      }
      pair<uint16_t, NDSInstrSet *> theBank(*iter, NewNDSInstrSet);
      BNKs.push_back(theBank);
    }
  }

  {
    //vector<uint16_t> vUniqueSeqs = vector<uint16_t>(seqFileIDs);
    //sort(vUniqueSeqs.begin(), vUniqueSeqs.end());
    //vector<uint16_t>::iterator new_end = unique(vUniqueBanks.begin(), vUniqueBanks.end());

    for (uint32_t i = 0; i < nSeqs; i++) {
      if (seqFileIDs[i] == std::numeric_limits<uint16_t>::max())
        continue;
      uint32_t offset = FAToff + 12 + seqFileIDs[i] * 0x10;
      uint32_t pSeqFatData = file->GetWord(offset) + baseOff;
      offset += 4;
      uint32_t fileSize = file->GetWord(offset);
      NDSSeq *NewNDSSeq = new NDSSeq(file, pSeqFatData, fileSize, seqNames[i]);
      if (!NewNDSSeq->LoadVGMFile()) {
        pRoot->AddLogItem(new LogItem(FormatString<wstring>(L"Failed to load NDSSeq at 0x%08x\n", pSeqFatData).c_str(),
                                      LOG_LEVEL_ERR,
                                      L"NDSScanner"));
      }

      VGMColl *coll = new VGMColl(seqNames[i]);
      coll->UseSeq(NewNDSSeq);
      uint32_t bnkIndex = 0;
      for (uint32_t j = 0; j < BNKs.size(); j++) {
        if (seqFileBnks[i] == BNKs[j].first) {
          bnkIndex = j;
          break;
        }
      }
//      NDSInstrSet *instrset = BNKs[bnkIndex].second;
      coll->AddInstrSet(BNKs[bnkIndex].second);
      for (int j = 0; j < 4; j++) {
        short WAnum = bnkWAs[seqFileBnks[i]][j];
        if (WAnum != -1)
          coll->AddSampColl(WAs[WAnum]);
      }
      if (!coll->Load()) {
        delete coll;
      }
    }
  }
  return SDATLength;
}




/*void NDSScanner::SearchForNDSSeq (RawFile* file)
{
	uint32_t nFileLength = file->size();
	for (uint32_t i=0; i+4<nFileLength; i++)
	{
		if ((*file)[i] == 'S' && (*file)[i+1] == 'S' && (*file)[i+2] == 'E' && (*file)[i+3] == 'Q')
		{
			//if (file->GetShort(i+10) == 0 && file->GetShort(i+16) == 0)
			//{
				NDSSeq* NewNDSSeq = new NDSSeq(file, i);
				NewNDSSeq->Load();
			//}
		}
	}
}*/