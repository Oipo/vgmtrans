#include "pch.h"
#include "VGMColl.h"

#include <utility>
#include "VGMSeq.h"
#include "VGMInstrSet.h"
#include "VGMSampColl.h"
#include "VGMSamp.h"
#include "VGMRgn.h"
#include "ScaleConversion.h"
#include "Root.h"

using namespace std;

DECLARE_MENU(VGMColl)

VGMColl::VGMColl(wstring theName)
    : VGMItem(),
      seq(nullptr),
      name(std::move(theName)) {
}

VGMColl::~VGMColl() = default;

void VGMColl::RemoveFileAssocs() {
  if (seq)
    seq->RemoveCollAssoc(this);
  for (auto & instrset : instrsets)
    instrset->RemoveCollAssoc(this);
  for (auto & sampcoll : sampcolls)
    sampcoll->RemoveCollAssoc(this);
  for (auto & miscfile : miscfiles)
    miscfile->RemoveCollAssoc(this);
}

const wstring *VGMColl::GetName() const {
  return &name;
}

void VGMColl::SetName(const wstring *newName) {
  name = *newName;
}

VGMSeq *VGMColl::GetSeq() const {
  return seq;
}

void VGMColl::UseSeq(VGMSeq *theSeq) {
  if (theSeq != nullptr)
    theSeq->AddCollAssoc(this);
  if (seq && (theSeq != seq))    //if we associated with a previous sequence
    seq->RemoveCollAssoc(this);
  seq = theSeq;
}

void VGMColl::AddInstrSet(VGMInstrSet *theInstrSet) {
  if (theInstrSet != nullptr) {
    theInstrSet->AddCollAssoc(this);
    instrsets.push_back(theInstrSet);
  }
}

void VGMColl::AddSampColl(VGMSampColl *theSampColl) {
  if (theSampColl != nullptr) {
    theSampColl->AddCollAssoc(this);
    sampcolls.push_back(theSampColl);
  }
}

void VGMColl::AddMiscFile(VGMFile *theMiscFile) {
  if (theMiscFile != nullptr) {
    theMiscFile->AddCollAssoc(this);
    miscfiles.push_back(theMiscFile);
  }
}


bool VGMColl::Load() {
  if (!LoadMain())
    return false;
  pRoot->AddVGMColl(this);
  return true;
}

void VGMColl::UnpackSampColl(DLSFile &dls, VGMSampColl *sampColl, vector<VGMSamp *> &finalSamps) {
  assert(sampColl != nullptr);

  size_t nSamples = sampColl->samples.size();
  for (size_t i = 0; i < nSamples; i++) {
    VGMSamp *samp = sampColl->samples[i];

    uint32_t bufSize;
    if (samp->ulUncompressedSize)
      bufSize = samp->ulUncompressedSize;
    else
      bufSize = ceil(static_cast<double>(samp->dataLength) * samp->GetCompressionRatio());
    uint8_t *uncompSampBuf = new uint8_t[bufSize];    //create a new memory space for the uncompressed wave
    samp->ConvertToStdWave(uncompSampBuf);            //and uncompress into that space

    uint16_t blockAlign = samp->bps / 8 * samp->channels;
    dls.AddWave(1, samp->channels, samp->rate, samp->rate * blockAlign, blockAlign,
                samp->bps, bufSize, uncompSampBuf, wstring2string(samp->name));
    finalSamps.push_back(samp);
  }
}

void VGMColl::UnpackSampColl(SynthFile &synthfile, VGMSampColl *sampColl, vector<VGMSamp *> &finalSamps) {
  assert(sampColl != nullptr);

  size_t nSamples = sampColl->samples.size();
  for (size_t i = 0; i < nSamples; i++) {
    VGMSamp *samp = sampColl->samples[i];

    uint32_t bufSize;
    if (samp->ulUncompressedSize)
      bufSize = samp->ulUncompressedSize;
    else
      bufSize = ceil(static_cast<double>(samp->dataLength) * samp->GetCompressionRatio());

    uint8_t *uncompSampBuf = new uint8_t[bufSize];    //create a new memory space for the uncompressed wave
    samp->ConvertToStdWave(uncompSampBuf);            //and uncompress into that space

    uint16_t blockAlign = samp->bps / 8 * samp->channels;
    SynthWave *wave = synthfile.AddWave(1, samp->channels, samp->rate, samp->rate * blockAlign, blockAlign,
                                        samp->bps, bufSize, uncompSampBuf, wstring2string(samp->name));
    finalSamps.push_back(samp);

    // If we don't have any loop information, then don't create a sampInfo structure for the Wave
    if (samp->loop.loopStatus == -1)
      return;

    SynthSampInfo *sampInfo = wave->AddSampInfo();
    if (samp->bPSXLoopInfoPrioritizing) {
      if (samp->loop.loopStart != 0 || samp->loop.loopLength != 0)
        sampInfo->SetLoopInfo(samp->loop, samp);
    }
    else
      sampInfo->SetLoopInfo(samp->loop, samp);

    double attenuation = (samp->volume != -1) ? ConvertLogScaleValToAtten(samp->volume) : 0;
    uint8_t unityKey = (samp->unityKey != std::numeric_limits<decltype(samp->unityKey)>::max()) ? samp->unityKey : 0x3C;
    short fineTune = samp->fineTune;
    sampInfo->SetPitchInfo(unityKey, fineTune, attenuation);
  }
}


bool VGMColl::CreateDLSFile(DLSFile &dls) {
  bool result = true;
  PreSynthFileCreation();
  result &= MainDLSCreation(dls);
  PostSynthFileCreation();
  return result;
}

SF2File *VGMColl::CreateSF2File() {
  SynthFile *synthfile = CreateSynthFile();
  if (synthfile == nullptr)
    return nullptr;
  SF2File *sf2file = new SF2File(synthfile);
  delete synthfile;
  return sf2file;
}

bool VGMColl::MainDLSCreation(DLSFile &dls) {
  vector<VGMSamp *> finalSamps;
  //we have to collect the finalSampColls in case sampcolls is empty but there are multiple instrSets with
  //embedded sampcolls
  vector<VGMSampColl *> finalSampColls;

  if (instrsets.empty() /*|| !sampcolls.size()*/)
    return false;

  // if there are independent SampColl(s) in the collection
  if (!sampcolls.empty()) {
    for (auto & sampcoll : sampcolls) {
      finalSampColls.push_back(sampcoll);
      UnpackSampColl(dls, sampcoll, finalSamps);
    }
  }
    // otherwise, the SampColl(s) are children of the InstrSet(s)
  else {
    for (auto & instrset : instrsets) {
      finalSampColls.push_back(instrset->sampColl);
      UnpackSampColl(dls, instrset->sampColl, finalSamps);
    }
  }

  if (finalSamps.empty())
    return false;


  for (auto set : instrsets) {
    for(auto vgminstr : set->aInstrs) {
      std::string _name = wstring2string(vgminstr->name);
      DLSInstr *newInstr = dls.AddInstr(vgminstr->bank, vgminstr->instrNum, _name);
      for(auto rgn : vgminstr->aRgns) {
        //				if (rgn->sampNum+1 > sampColl->samples.size())	//does thereferenced sample exist?
        //					continue;

        // Determine the SampColl associated with this rgn.  If there's an explicit pointer to it, use that.
        VGMSampColl *sampColl = rgn->sampCollPtr;
        if (!sampColl) {
          // If rgn is of an InstrSet with an embedded SampColl, use that SampColl.
          if (dynamic_cast<VGMInstrSet*>(rgn->vgmfile)->sampColl)
            sampColl = dynamic_cast<VGMInstrSet*>(rgn->vgmfile)->sampColl;

            // If that does not exist, assume the first SampColl
          else
            sampColl = finalSampColls[0];
        }

        // Determine the sample number within the rgn's associated SampColl
        size_t realSampNum;
        // If a sample offset is provided, then find the sample number based on this offset.
        // see sampOffset declaration in header file for more info.
        if (rgn->sampOffset != std::numeric_limits<decltype(rgn->sampOffset)>::max()) {
          bool bFoundIt = false;
          for (uint32_t s = 0; s < sampColl->samples.size(); s++) {                            //for every sample
            if (rgn->sampOffset == sampColl->samples[s]->dwOffset - sampColl->dwOffset - sampColl->sampDataOffset) {
              realSampNum = s;

              //samples[m]->loop.loopStart = parInstrSet->aInstrs[i]->aRgns[k]->loop.loopStart;
              //samples[m]->loop.loopLength = (samples[m]->dataLength) - (parInstrSet->aInstrs[i]->aRgns[k]->loop.loopStart); //[aInstrs[i]->aRegions[k]->sample_num]->dwUncompSize/2) - ((aInstrs[i]->aRegions[k]->loop_point*28)/16); //to end of sample
              bFoundIt = true;
              break;
            }
          }
          if (!bFoundIt) {
            auto message = FormatString<wstring>(L"Could not match rgn with sampOffset %X to a sample with that offset.\n  (InstrSet, Instr, Rgn)", rgn->sampOffset); //TODO
            pRoot->AddLogItem(new LogItem(message, LOG_LEVEL_ERR, L"VGMColl"));
            realSampNum = 0;
          }
        }
          // Otherwise, the sample number should be explicitly defined in the rgn.
        else
          realSampNum = rgn->sampNum;


        // Determine the sampCollNum (index into our finalSampColls vector)
        unsigned int sampCollNum;
        for (unsigned int k = 0; k < finalSampColls.size(); k++) {
          if (finalSampColls[k] == sampColl)
            sampCollNum = k;
        }
        //   now we add the number of samples from the preceding SampColls to the value to get the real sampNum
        //   in the final DLS file.
        for (unsigned int k = 0; k < sampCollNum; k++)
          realSampNum += finalSampColls[k]->samples.size();


        // For collections with multiple SampColls
        // If a SampColl ptr is given, find the SampColl and adjust the sample number of the region
        // to compensate for all preceding SampColl samples.
        //if (rgn->sampCollNum == -1)	//if a sampCollPtr is defined
        //{
        //	// find the sampColl's index in samplecolls (the sampCollNum, effectively)
        //	for (uint32_t i=0; i < finalSampColls.size(); i++)
        //	{
        //		if (finalSampColls[i] == sampColl)
        //			rgn->sampCollNum = i;
        //	}
        //}
        //if (rgn->sampCollNum != -1)		//if a sampCollNum is defined
        //{									//then sampNum represents the sample number in the specific sample collection
        //	for (int k=0; k < rgn->sampCollNum; k++)
        //		realSampNum += finalSampColls[k]->samples.size();	//so now we add all previous sample collection samples to the value to get the real (absolute) sampNum
        //}


        DLSRgn *newRgn = newInstr->AddRgn();
        newRgn->SetRanges(rgn->keyLow, rgn->keyHigh,
                          rgn->velLow, rgn->velHigh);
        newRgn->SetWaveLinkInfo(0, 0, 1, realSampNum);

        if (realSampNum >= finalSamps.size()) {
          wchar_t log[256];
          swprintf(log, 256, L"Sample %u does not exist.", realSampNum);
          pRoot->AddLogItem(new LogItem(log, LOG_LEVEL_ERR, L"VGMColl"));
          realSampNum = finalSamps.size() - 1;
        }

        VGMSamp *samp = finalSamps[realSampNum];//sampColl->samples[rgn->sampNum];
        DLSWsmp *newWsmp = newRgn->AddWsmp();

        // This is a really loopy way of determining the loop information, pardon the pun.  However, it works.
        // There might be a way to simplify this, but I don't want to test out whether another method breaks anything just yet
        // Use the sample's loopStatus to determine if a loop occurs.  If it does, see if the sample provides loop info
        // (gathered during ADPCM > PCM conversion.  If the sample doesn't provide loop offset info, then use the region's
        // loop info.
        if (samp->bPSXLoopInfoPrioritizing) {
          if (samp->loop.loopStatus != -1) {
            if (samp->loop.loopStart != 0 || samp->loop.loopLength != 0)
              newWsmp->SetLoopInfo(samp->loop, samp);
            else {
              rgn->loop.loopStatus = samp->loop.loopStatus;
              newWsmp->SetLoopInfo(rgn->loop, samp);
            }
          }
          else throw;
        }
          // The normal method: First, we check if the rgn has loop info defined.
          // If it doesn't, then use the sample's loop info.
        else if (rgn->loop.loopStatus == -1) {
          if (samp->loop.loopStatus != -1)
            newWsmp->SetLoopInfo(samp->loop, samp);
          else throw;
        }
        else
          newWsmp->SetLoopInfo(rgn->loop, samp);

        uint8_t realUnityKey;
        if (rgn->unityKey == std::numeric_limits<decltype(rgn->unityKey)>::max())
          realUnityKey = samp->unityKey;
        else
          realUnityKey = rgn->unityKey;
        if (realUnityKey == std::numeric_limits<decltype(realUnityKey)>::max())
          realUnityKey = 0x3C;

        short realFineTune;
        if (rgn->fineTune == 0)
          realFineTune = samp->fineTune;
        else
          realFineTune = rgn->fineTune;

        long realAttenuation;
        if (rgn->volume == -1 && samp->volume == -1)
          realAttenuation = 0;
        else if (rgn->volume == -1)
          realAttenuation = static_cast<long>(-(ConvertLogScaleValToAtten(samp->volume) * DLS_DECIBEL_UNIT * 10));
        else
          realAttenuation = static_cast<long>(-(ConvertLogScaleValToAtten(rgn->volume) * DLS_DECIBEL_UNIT * 10));

        long convAttack = roundi(SecondsToTimecents(rgn->attack_time) * 65536);
        long convDecay = roundi(SecondsToTimecents(rgn->decay_time) * 65536);
        long convSustainLev;
        if (rgn->sustain_level == -1)
          convSustainLev = 0x03e80000;        //sustain at full if no sustain level provided
        else {
          //the DLS envelope is a range from 0 to -96db.
          double attenInDB = ConvertLogScaleValToAtten(rgn->sustain_level);
          convSustainLev = static_cast<long>(((96.0 - attenInDB) / 96.0) * 0x03e80000);
        }

        long convRelease = roundi(SecondsToTimecents(rgn->release_time) * 65536);

        DLSArt *newArt = newRgn->AddArt();
        newArt->AddPan(ConvertPercentPanTo10thPercentUnits(rgn->pan) * 65536);
        newArt->AddADSR(convAttack, 0, convDecay, convSustainLev, convRelease, 0);

        newWsmp->SetPitchInfo(realUnityKey, realFineTune, realAttenuation);
      }
    }
  }
  return true;
}


SynthFile *VGMColl::CreateSynthFile() {
  PreSynthFileCreation();

  SynthFile *synthfile = new SynthFile("SynthFile"/**this->instrsets[0]->GetName()*/);


  vector<VGMSamp *> finalSamps;
  //we have to collect the finalSampColls in case sampcolls is empty but there are multiple instrSets with
  //embedded sampcolls
  vector<VGMSampColl *> finalSampColls;

  if (instrsets.empty() /*|| !sampcolls.size()*/) {
    delete synthfile;
    PostSynthFileCreation();
    return nullptr;
  }

  // if there are independent SampColl(s) in the collection
  if (!sampcolls.empty()) {
    for (auto & sampcoll : sampcolls) {
      finalSampColls.push_back(sampcoll);
      UnpackSampColl(*synthfile, sampcoll, finalSamps);
    }
  }
    // otherwise, the SampColl(s) are children of the InstrSet(s)
  else {
    for (auto & instrset : instrsets) {
      finalSampColls.push_back(instrset->sampColl);
      UnpackSampColl(*synthfile, instrset->sampColl, finalSamps);
    }
  }

  if (finalSamps.empty()) {
    delete synthfile;
    PostSynthFileCreation();
    return nullptr;
  }


  for(auto set : instrsets) {
    for(auto vgminstr : set->aInstrs) {
      size_t nRgns = vgminstr->aRgns.size();
      if (nRgns == 0)                                //do not write an instrument if it has no regions
        continue;
      SynthInstr *newInstr = synthfile->AddInstr(vgminstr->bank, vgminstr->instrNum);
      for(auto rgn : vgminstr->aRgns) {
        //				if (rgn->sampNum+1 > sampColl->samples.size())	//does thereferenced sample exist?
        //					continue;

        // Determine the SampColl associated with this rgn.  If there's an explicit pointer to it, use that.
        VGMSampColl *sampColl = rgn->sampCollPtr;
        if (!sampColl) {
          // If rgn is of an InstrSet with an embedded SampColl, use that SampColl.
          if (dynamic_cast<VGMInstrSet*>(rgn->vgmfile)->sampColl)
            sampColl = dynamic_cast<VGMInstrSet*>(rgn->vgmfile)->sampColl;

            // If that does not exist, assume the first SampColl
          else
            sampColl = finalSampColls[0];
        }

        // Determine the sample number within the rgn's associated SampColl
        size_t realSampNum;
        // If a sample offset is provided, then find the sample number based on this offset.
        // see sampOffset declaration in header file for more info.
        if (rgn->sampOffset != std::numeric_limits<decltype(rgn->sampOffset)>::max()) {
          bool bFoundIt = false;
          for (uint32_t s = 0; s < sampColl->samples.size(); s++) {                            //for every sample
            if (rgn->sampOffset == sampColl->samples[s]->dwOffset - sampColl->dwOffset - sampColl->sampDataOffset) {
              realSampNum = s;

              //samples[m]->loop.loopStart = parInstrSet->aInstrs[i]->aRgns[k]->loop.loopStart;
              //samples[m]->loop.loopLength = (samples[m]->dataLength) - (parInstrSet->aInstrs[i]->aRgns[k]->loop.loopStart); //[aInstrs[i]->aRegions[k]->sample_num]->dwUncompSize/2) - ((aInstrs[i]->aRegions[k]->loop_point*28)/16); //to end of sample
              bFoundIt = true;
              break;
            }
          }
          if (!bFoundIt) {
            auto message = FormatString<wstring>(L"Could not match rgn with sampOffset %X to a sample with that offset.\n  (InstrSet, Instr, Rgn)", rgn->sampOffset); //TODO instrset, VGMRgn and instr ids
            pRoot->AddLogItem(new LogItem(message, LOG_LEVEL_ERR, L"VGMColl"));
            realSampNum = 0;
          }
        }
          // Otherwise, the sample number should be explicitly defined in the rgn.
        else
          realSampNum = rgn->sampNum;


        // Determine the sampCollNum (index into our finalSampColls vector)
        unsigned int sampCollNum = finalSampColls.size();
        for (uint32_t l = 0; l < finalSampColls.size(); l++) {
          if (finalSampColls[l] == sampColl)
            sampCollNum = l;
        }
        if (sampCollNum == finalSampColls.size()) {
          pRoot->AddLogItem(new LogItem(L"SampColl does not exist.", LOG_LEVEL_ERR, L"VGMColl"));
          delete synthfile;
          PostSynthFileCreation();
          return nullptr;
        }
        //   now we add the number of samples from the preceding SampColls to the value to get the real sampNum
        //   in the final DLS file.
        for (uint32_t k = 0; k < sampCollNum; k++)
          realSampNum += finalSampColls[k]->samples.size();

        SynthRgn *newRgn = newInstr->AddRgn();
        newRgn->SetRanges(rgn->keyLow, rgn->keyHigh,
                          rgn->velLow, rgn->velHigh);
        newRgn->SetWaveLinkInfo(0, 0, 1, realSampNum);

        if (realSampNum >= finalSamps.size()) {
          wchar_t log[256];
          swprintf(log, 256, L"Sample %u does not exist.", realSampNum);
          pRoot->AddLogItem(new LogItem(log, LOG_LEVEL_ERR, L"VGMColl"));
          realSampNum = finalSamps.size() - 1;
        }

        VGMSamp *samp = finalSamps[realSampNum];//sampColl->samples[rgn->sampNum];
        SynthSampInfo *sampInfo = newRgn->AddSampInfo();

        // This is a really loopy way of determining the loop information, pardon the pun.  However, it works.
        // There might be a way to simplify this, but I don't want to test out whether another method breaks anything just yet
        // Use the sample's loopStatus to determine if a loop occurs.  If it does, see if the sample provides loop info
        // (gathered during ADPCM > PCM conversion.  If the sample doesn't provide loop offset info, then use the region's
        // loop info.
        if (samp->bPSXLoopInfoPrioritizing) {
          if (samp->loop.loopStatus != -1) {
            if (samp->loop.loopStart != 0 || samp->loop.loopLength != 0)
              sampInfo->SetLoopInfo(samp->loop, samp);
            else {
              rgn->loop.loopStatus = samp->loop.loopStatus;
              sampInfo->SetLoopInfo(rgn->loop, samp);
            }
          }
          else {
            delete synthfile;
            throw;
          }
        }
        // The normal method: First, we check if the rgn has loop info defined.
        // If it doesn't, then use the sample's loop info.
        else if (rgn->loop.loopStatus == -1) {
          if (samp->loop.loopStatus != -1)
            sampInfo->SetLoopInfo(samp->loop, samp);
          else {
            delete synthfile;
            throw;
          }
        }
        else
          sampInfo->SetLoopInfo(rgn->loop, samp);

        uint8_t realUnityKey;
        if (rgn->unityKey == std::numeric_limits<decltype(rgn->unityKey)>::max())
          realUnityKey = samp->unityKey;
        else
          realUnityKey = rgn->unityKey;
        if (realUnityKey == std::numeric_limits<decltype(realUnityKey)>::max())
          realUnityKey = 0x3C;

        short realFineTune;
        if (rgn->fineTune == 0)
          realFineTune = samp->fineTune;
        else
          realFineTune = rgn->fineTune;

        double attenuation;
        if (rgn->volume != -1)
          attenuation = ConvertLogScaleValToAtten(rgn->volume);
        else if (samp->volume != -1)
          attenuation = ConvertLogScaleValToAtten(samp->volume);
        else
          attenuation = 0;

        double sustainLevAttenDb;
        if (rgn->sustain_level == -1)
          sustainLevAttenDb = 0.0;
        else
          sustainLevAttenDb = ConvertPercentAmplitudeToAttenDB_SF2(rgn->sustain_level);

        SynthArt *newArt = newRgn->AddArt();
        newArt->AddPan(rgn->pan);
        newArt->AddADSR(rgn->attack_time, rgn->attack_transform, rgn->decay_time,
                        sustainLevAttenDb, rgn->sustain_time, rgn->release_time, rgn->release_transform);

        sampInfo->SetPitchInfo(realUnityKey, realFineTune, attenuation);
      }
    }
  }
  PostSynthFileCreation();
  return synthfile;
}


bool VGMColl::OnSaveAllDLS() {
  wstring dirpath = pRoot->UI_GetSaveDirPath();
  if (dirpath.length() != 0) {
    DLSFile dlsfile;
    wstring filepath = dirpath + L"\\" + ConvertToSafeFileName(this->name) + L".dls";
    if (CreateDLSFile(dlsfile)) {
      if (!dlsfile.SaveDLSFile(filepath))
        pRoot->AddLogItem(new LogItem(std::wstring(L"Failed to save DLS file"), LOG_LEVEL_ERR, L"VGMColl"));
    }
    else
      pRoot->AddLogItem(new LogItem(std::wstring(L"Failed to save DLS file"), LOG_LEVEL_ERR, L"VGMColl"));

    if (this->seq != nullptr) {
      filepath = dirpath + L"\\" + ConvertToSafeFileName(this->name) + L".mid";
      if (!this->seq->SaveAsMidi(filepath))
        pRoot->AddLogItem(new LogItem(std::wstring(L"Failed to save MIDI file"), LOG_LEVEL_ERR, L"VGMColl"));
    }
  }
  return true;
}

bool VGMColl::OnSaveAllSF2() {
  wstring dirpath = pRoot->UI_GetSaveDirPath();
  if (dirpath.length() != 0) {
    wstring filepath = dirpath + L"\\" + ConvertToSafeFileName(this->name) + L".sf2";
    SF2File *sf2file = CreateSF2File();
    if (sf2file != nullptr) {
      if (!sf2file->SaveSF2File(filepath))
        pRoot->AddLogItem(new LogItem(std::wstring(L"Failed to save SF2 file"), LOG_LEVEL_ERR, L"VGMColl"));
      delete sf2file;
    }
    else
      pRoot->AddLogItem(new LogItem(std::wstring(L"Failed to save SF2 file"), LOG_LEVEL_ERR, L"VGMColl"));

    if (this->seq != nullptr) {
      filepath = dirpath + L"\\" + ConvertToSafeFileName(this->name) + L".mid";
      if (!this->seq->SaveAsMidi(filepath))
        pRoot->AddLogItem(new LogItem(std::wstring(L"Failed to save MIDI file"), LOG_LEVEL_ERR, L"VGMColl"));
    }
  }
  return true;
}
