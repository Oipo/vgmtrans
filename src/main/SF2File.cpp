#include "pch.h"
#include "SF2File.h"
#include "VGMInstrSet.h"
#include "SynthFile.h"
#include "ScaleConversion.h"
#include "Root.h"

using namespace std;

SF2InfoListChunk::SF2InfoListChunk(string name)
    : LISTChunk("INFO") {
  // Create a date string
  time_t current_time = time(nullptr);
  char *c_time_string = ctime(&current_time);

  // Add the child info chunks
  Chunk *ifilCk = new Chunk("ifil");
  sfVersionTag versionTag;        //soundfont version 2.01
  versionTag.wMajor = 2;
  versionTag.wMinor = 1;
  ifilCk->SetData(&versionTag, sizeof(versionTag));
  AddChildChunk(ifilCk);
  AddChildChunk(new SF2StringChunk("isng", "EMU8000"));
  AddChildChunk(new SF2StringChunk("INAM", name));
  AddChildChunk(new SF2StringChunk("ICRD", string(c_time_string)));
  AddChildChunk(new SF2StringChunk("ISFT", string("VGMTrans " + string(VERSION))));
}


//  *******
//  SF2File
//  *******

SF2File::SF2File(SynthFile *synthfile)
    : RiffFile(synthfile->name, "sfbk") {

  //***********
  // INFO chunk
  //***********
  AddChildChunk(new SF2InfoListChunk(name));

  // sdta chunk and its child smpl chunk containing all samples
  LISTChunk *sdtaCk = new LISTChunk("sdta");
  Chunk *smplCk = new Chunk("smpl");

  // Concatanate all of the samples together and add the result to the smpl chunk data
  size_t numWaves = synthfile->vWaves.size();
  smplCk->size = 0;
  for (size_t i = 0; i < numWaves; i++) {
    SynthWave *wave = synthfile->vWaves[i];
    wave->ConvertTo16bitSigned();
    smplCk->size += wave->dataSize + (46 * 2);    // plus the 46 padding samples required by sf2 spec
  }
  smplCk->data = new uint8_t[smplCk->size];
  uint32_t bufPtr = 0;
  for (size_t i = 0; i < numWaves; i++) {
    SynthWave *wave = synthfile->vWaves[i];

    memcpy(smplCk->data + bufPtr, wave->data, wave->dataSize);
    memset(smplCk->data + bufPtr + wave->dataSize, 0, 46 * 2);
    bufPtr += wave->dataSize + (46 * 2);        // plus the 46 padding samples required by sf2 spec
  }

  sdtaCk->AddChildChunk(smplCk);
  this->AddChildChunk(sdtaCk);

  //***********
  // pdta chunk
  //***********

  LISTChunk *pdtaCk = new LISTChunk("pdta");

  //***********
  // phdr chunk
  //***********
  Chunk *phdrCk = new Chunk("phdr");
  size_t numInstrs = synthfile->vInstrs.size();
  phdrCk->size = (numInstrs + 1) * sizeof(sfPresetHeader);
  phdrCk->data = new uint8_t[phdrCk->size];

  for (size_t i = 0; i < numInstrs; i++) {
    SynthInstr *instr = synthfile->vInstrs[i];

    sfPresetHeader presetHdr{};
    memcpy(presetHdr.achPresetName, instr->name.c_str(), min(instr->name.length(), 20ul));
    presetHdr.wPreset = instr->ulInstrument;

    // Despite being a 16-bit value, SF2 only supports banks up to 127. Since
    // it's pretty common to have either MSB or LSB be 0, we'll use whatever
    // one is not zero, with preference for MSB.
    auto bank16 = instr->ulBank;

    if ((bank16 & 0xFF00) == 0) {
      presetHdr.wBank = bank16 & 0x7F;
    }
    else {
      presetHdr.wBank = (bank16 >> 8) & 0x7F;
    }
    presetHdr.wPresetBagNdx = i;
    presetHdr.dwLibrary = 0;
    presetHdr.dwGenre = 0;
    presetHdr.dwMorphology = 0;

    memcpy(phdrCk->data + (i * sizeof(sfPresetHeader)), &presetHdr, sizeof(sfPresetHeader));
  }
  //  add terminal sfPresetBag
  sfPresetHeader presetHdr{};
  presetHdr.wPresetBagNdx = numInstrs;
  memcpy(phdrCk->data + (numInstrs * sizeof(sfPresetHeader)), &presetHdr, sizeof(sfPresetHeader));
  pdtaCk->AddChildChunk(phdrCk);

  //***********
  // pbag chunk
  //***********
  Chunk *pbagCk = new Chunk("pbag");
  const size_t ITEMS_IN_PGEN = 2;
  pbagCk->size = ((numInstrs + 1) * sizeof(sfPresetBag));
  pbagCk->data = new uint8_t[pbagCk->size];
  for (size_t i = 0; i < numInstrs; i++) {
    sfPresetBag presetBag{};
    presetBag.wGenNdx = i * ITEMS_IN_PGEN;
    presetBag.wModNdx = 0;

    memcpy(pbagCk->data + (i * sizeof(sfPresetBag)), &presetBag, sizeof(sfPresetBag));
  }
  //  add terminal sfPresetBag
  sfPresetBag presetBag{};
  presetBag.wGenNdx = numInstrs * ITEMS_IN_PGEN;
  memcpy(pbagCk->data + (numInstrs * sizeof(sfPresetBag)), &presetBag, sizeof(sfPresetBag));
  pdtaCk->AddChildChunk(pbagCk);

  //***********
  // pmod chunk
  //***********
  Chunk *pmodCk = new Chunk("pmod");
  //  create the terminal field
  sfModList modList{};
  pmodCk->SetData(&modList, sizeof(sfModList));
  //modList.sfModSrcOper = cc1_Mod;
  //modList.sfModDestOper = startAddrsOffset;
  //modList.modAmount = 0;
  //modList.sfModAmtSrcOper = cc1_Mod;
  //modList.sfModTransOper = linear;
  pdtaCk->AddChildChunk(pmodCk);

  //***********
  // pgen chunk
  //***********
  Chunk *pgenCk = new Chunk("pgen");
  //pgenCk->size = (synthfile->vInstrs.size()+1) * sizeof(sfGenList);
  pgenCk->size = ((synthfile->vInstrs.size() * sizeof(sfGenList) * ITEMS_IN_PGEN) + sizeof(sfGenList));
  pgenCk->data = new uint8_t[pgenCk->size];
  uint32_t dataPtr = 0;
  for (size_t i = 0; i < numInstrs; i++) {
    sfGenList genList{};

    // reverbEffectsSend
    genList.sfGenOper = reverbEffectsSend;
    genList.genAmount.shAmount = 250;
    memcpy(pgenCk->data + dataPtr, &genList, sizeof(sfGenList));
    dataPtr += sizeof(sfGenList);

    genList.sfGenOper = instrument;
    genList.genAmount.wAmount = i;
    memcpy(pgenCk->data + dataPtr, &genList, sizeof(sfGenList));
    dataPtr += sizeof(sfGenList);
  }
  //  add terminal sfGenList
  sfGenList genList{};
  memcpy(pgenCk->data + dataPtr, &genList, sizeof(sfGenList));

  pdtaCk->AddChildChunk(pgenCk);

  //***********
  // inst chunk
  //***********
  Chunk *instCk = new Chunk("inst");
  instCk->size = (synthfile->vInstrs.size() + 1) * sizeof(sfInst);
  instCk->data = new uint8_t[instCk->size];
  size_t rgnCounter = 0;
  for (size_t i = 0; i < numInstrs; i++) {
    SynthInstr *instr = synthfile->vInstrs[i];

    sfInst inst{};
    memcpy(inst.achInstName, instr->name.c_str(), min(instr->name.length(), 20ul));
    inst.wInstBagNdx = rgnCounter;
    rgnCounter += instr->vRgns.size();

    memcpy(instCk->data + (i * sizeof(sfInst)), &inst, sizeof(sfInst));
  }
  //  add terminal sfInst
  sfInst inst{};
  inst.wInstBagNdx = rgnCounter;
  memcpy(instCk->data + (numInstrs * sizeof(sfInst)), &inst, sizeof(sfInst));
  pdtaCk->AddChildChunk(instCk);

  //***********
  // ibag chunk - stores all zones (regions) for instruments
  //***********
  Chunk *ibagCk = new Chunk("ibag");

  size_t numRgns = 0;
  for (size_t i = 0; i < numInstrs; i++)
    numRgns += synthfile->vInstrs[i]->vRgns.size();

  ibagCk->size = ((numRgns + 1) * sizeof(sfInstBag));
  ibagCk->data = new uint8_t[ibagCk->size];

  rgnCounter = 0;
  int instGenCounter = 0;
  for(auto &instr : synthfile->vInstrs) {
    for(uint64_t i = 0; i < instr->vRgns.size(); i++) {
      sfInstBag instBag{};
      instBag.wInstGenNdx = instGenCounter;
      instGenCounter += 11;
      instBag.wInstModNdx = 0;

      memcpy(ibagCk->data + (rgnCounter++ * sizeof(sfInstBag)), &instBag, sizeof(sfInstBag));
    }
  }
  //  add terminal sfInstBag
  sfInstBag instBag{};
  instBag.wInstGenNdx = instGenCounter;
  instBag.wInstModNdx = 0;
  memcpy(ibagCk->data + (rgnCounter * sizeof(sfInstBag)), &instBag, sizeof(sfInstBag));
  pdtaCk->AddChildChunk(ibagCk);

  //***********
  // imod chunk
  //***********
  Chunk *imodCk = new Chunk("imod");
  //  create the terminal field
  memset(&modList, 0, sizeof(sfModList));
  imodCk->SetData(&modList, sizeof(sfModList));
  pdtaCk->AddChildChunk(imodCk);

  //***********
  // igen chunk
  //***********
  Chunk *igenCk = new Chunk("igen");
  igenCk->size = (numRgns * sizeof(sfInstGenList) * 11) + sizeof(sfInstGenList);
  igenCk->data = new uint8_t[igenCk->size];
  dataPtr = 0;
  for(auto &instr : synthfile->vInstrs) {
    for(auto &rgn : instr->vRgns) {
      sfInstGenList instGenList{};
      // Key range - (if exists) this must be the first chunk
      instGenList.sfGenOper = keyRange;
      instGenList.genAmount.ranges.byLo = rgn->usKeyLow;
      instGenList.genAmount.ranges.byHi = rgn->usKeyHigh;
      memcpy(igenCk->data + dataPtr, &instGenList, sizeof(sfInstGenList));
      dataPtr += sizeof(sfInstGenList);

      if (rgn->usVelHigh)    // 0 means 'not set', fixes TriAce instruments
      {
        // Velocity range (if exists) this must be the next chunk
        instGenList.sfGenOper = velRange;
        instGenList.genAmount.ranges.byLo = rgn->usVelLow;
        instGenList.genAmount.ranges.byHi = rgn->usVelHigh;
        memcpy(igenCk->data + dataPtr, &instGenList, sizeof(sfInstGenList));
        dataPtr += sizeof(sfInstGenList);
      }

      // initialAttenuation
      instGenList.sfGenOper = initialAttenuation;
      instGenList.genAmount.shAmount = static_cast<int16_t>(rgn->sampinfo->attenuation * 10);
      memcpy(igenCk->data + dataPtr, &instGenList, sizeof(sfInstGenList));
      dataPtr += sizeof(sfInstGenList);

      // pan
      instGenList.sfGenOper = pan;
      instGenList.genAmount.shAmount = static_cast<int16_t>(ConvertPercentPanTo10thPercentUnits(rgn->art->pan));
      memcpy(igenCk->data + dataPtr, &instGenList, sizeof(sfInstGenList));
      dataPtr += sizeof(sfInstGenList);

      // sampleModes
      instGenList.sfGenOper = sampleModes;
      instGenList.genAmount.wAmount = rgn->sampinfo->cSampleLoops;
      memcpy(igenCk->data + dataPtr, &instGenList, sizeof(sfInstGenList));
      dataPtr += sizeof(sfInstGenList);

      // overridingRootKey
      instGenList.sfGenOper = overridingRootKey;
      instGenList.genAmount.wAmount = rgn->sampinfo->usUnityNote;
      memcpy(igenCk->data + dataPtr, &instGenList, sizeof(sfInstGenList));
      dataPtr += sizeof(sfInstGenList);

      // attackVolEnv
      instGenList.sfGenOper = attackVolEnv;
      instGenList.genAmount.shAmount =
          (rgn->art->attack_time == 0) ? -32768 : roundi(SecondsToTimecents(rgn->art->attack_time));
      memcpy(igenCk->data + dataPtr, &instGenList, sizeof(sfInstGenList));
      dataPtr += sizeof(sfInstGenList);

      // decayVolEnv
      instGenList.sfGenOper = decayVolEnv;
      instGenList.genAmount.shAmount =
          (rgn->art->decay_time == 0) ? -32768 : roundi(SecondsToTimecents(rgn->art->decay_time));
      memcpy(igenCk->data + dataPtr, &instGenList, sizeof(sfInstGenList));
      dataPtr += sizeof(sfInstGenList);

      // sustainVolEnv
      instGenList.sfGenOper = sustainVolEnv;
      if (rgn->art->sustain_lev > 100.0)
        rgn->art->sustain_lev = 100.0;
      instGenList.genAmount.shAmount = static_cast<int16_t>(rgn->art->sustain_lev * 10);
      memcpy(igenCk->data + dataPtr, &instGenList, sizeof(sfInstGenList));
      dataPtr += sizeof(sfInstGenList);

      // releaseVolEnv
      instGenList.sfGenOper = releaseVolEnv;
      instGenList.genAmount.shAmount =
          (rgn->art->release_time == 0) ? -32768 : roundi(SecondsToTimecents(rgn->art->release_time));
      memcpy(igenCk->data + dataPtr, &instGenList, sizeof(sfInstGenList));
      dataPtr += sizeof(sfInstGenList);

      // reverbEffectsSend
      //instGenList.sfGenOper = reverbEffectsSend;
      //instGenList.genAmount.shAmount = 800;
      //memcpy(pgenCk->data + dataPtr, &instGenList, sizeof(sfInstGenList));
      //dataPtr += sizeof(sfInstGenList);

      // sampleID - this is the terminal chunk
      instGenList.sfGenOper = sampleID;
      instGenList.genAmount.wAmount = rgn->tableIndex;
      memcpy(igenCk->data + dataPtr, &instGenList, sizeof(sfInstGenList));
      dataPtr += sizeof(sfInstGenList);

      //int numConnBlocks = rgn->art->vConnBlocks.size();
      //for (int k = 0; k < numConnBlocks; k++)
      //{
      //	SynthConnectionBlock* connBlock = rgn->art->vConnBlocks[k];
      //	connBlock->
      //}
    }
  }
  //  add terminal sfInstBag
  sfInstGenList instGenList{};
  memcpy(igenCk->data + dataPtr, &instGenList, sizeof(sfInstGenList));
  //memset(ibagCk->data + (numRgns*sizeof(sfInstBag)), 0, sizeof(sfInstBag));
  //igenCk->SetData(&genList, sizeof(sfGenList));
  pdtaCk->AddChildChunk(igenCk);

  //***********
  // shdr chunk
  //***********
  Chunk *shdrCk = new Chunk("shdr");

  size_t numSamps = synthfile->vWaves.size();
  shdrCk->size = (numSamps + 1) * sizeof(sfSample);
  shdrCk->data = new uint8_t[shdrCk->size];


  uint32_t sampOffset = 0;
  for (size_t i = 0; i < numSamps; i++) {
    SynthWave *wave = synthfile->vWaves[i];

    sfSample samp{};
    memcpy(samp.achSampleName, wave->name.c_str(), min(wave->name.length(), 20ul));
    samp.dwStart = sampOffset;
    samp.dwEnd = samp.dwStart + (wave->dataSize / sizeof(uint16_t));
    sampOffset = samp.dwEnd + 46;        // plus the 46 padding samples required by sf2 spec

    // Search through all regions for an associated sampInfo structure with this sample
    SynthSampInfo *sampInfo = nullptr;
    for (size_t j = 0; j < numInstrs; j++) {
      SynthInstr *instr = synthfile->vInstrs[j];

      for(auto &rgn : instr->vRgns) {
        if (rgn->tableIndex == i && rgn->sampinfo != nullptr) {
          sampInfo = rgn->sampinfo;
          break;
        }
      }
      if (sampInfo != nullptr)
        break;
    }
    //  If we didn't find a rgn association, then it should be in the SynthWave structure.
    if (sampInfo == nullptr)
      sampInfo = wave->sampinfo;
    assert (sampInfo != nullptr);

    assert(sampInfo->sFineTune >= std::numeric_limits<char>::min() &&
           sampInfo->sFineTune <= std::numeric_limits<char>::max());

    samp.dwStartloop = samp.dwStart + sampInfo->ulLoopStart;
    samp.dwEndloop = samp.dwStartloop + sampInfo->ulLoopLength;
    samp.dwSampleRate = wave->dwSamplesPerSec;
    samp.byOriginalKey = sampInfo->usUnityNote;
    samp.chCorrection = sampInfo->sFineTune;
    samp.wSampleLink = 0;
    samp.sfSampleType = monoSample;

    memcpy(shdrCk->data + (i * sizeof(sfSample)), &samp, sizeof(sfSample));
  }

  //  add terminal sfSample
  memset(shdrCk->data + (numSamps * sizeof(sfSample)), 0, sizeof(sfSample));
  pdtaCk->AddChildChunk(shdrCk);

  this->AddChildChunk(pdtaCk);

}


uint8_t *SF2File::SaveToMem() {
  uint32_t _size = this->GetSize();
  uint8_t *buf = new uint8_t[_size];
  this->Write(buf);
  return buf;
}

bool SF2File::SaveSF2File(const std::wstring &filepath) {
  uint32_t _size = this->GetSize();
  uint8_t *buf = this->SaveToMem();
  bool result = pRoot->UI_WriteBufferToFile(filepath, buf, _size);
  delete[] buf;
  return result;
}
