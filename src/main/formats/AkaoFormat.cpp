#include "pch.h"
#include "AkaoFormat.h"
#include "AkaoSeq.h"
#include "AkaoInstr.h"
#include "PSXSPU.h"

using namespace std;

bool AkaoColl::LoadMain() {
  auto *instrset = dynamic_cast<AkaoInstrSet *>(instrsets[0]);
  auto *sampcoll = dynamic_cast<AkaoSampColl *>(sampcolls[0]);

  //Set the sample numbers of each region using the articulation data references of each region
  for (auto & aInstr : instrset->aInstrs) {
    auto *instr = dynamic_cast<AkaoInstr *>(aInstr);
    std::vector<VGMRgn *> *rgns = &instr->aRgns;
    for (auto & j : *rgns) {
      auto *rgn = dynamic_cast<AkaoRgn *>(j);

      AkaoArt *art;

      int64_t negativeTest = rgn->artNum - sampcoll->starting_art_id;
      if(negativeTest < 0) {
        continue;
      }

      if (rgn->artNum - sampcoll->starting_art_id >= 200) {
        pRoot->AddLogItem(new LogItem(std::wstring(L"Articulation reference does not exist in the samp collection"),
                                      LOG_LEVEL_ERR,
                                      L"AkaoColl"));
        art = &sampcoll->akArts.front();
      }

      else if (rgn->artNum - sampcoll->starting_art_id >= sampcoll->akArts.size()) {
        pRoot->AddLogItem(new LogItem(std::wstring(L"referencing an articulation that was not loaded"),
                                      LOG_LEVEL_ERR,
                                      L"AkaoColl"));
        art = &sampcoll->akArts.back();
      }
      else
        art = &sampcoll->akArts[rgn->artNum - sampcoll->starting_art_id];
      rgn->SetSampNum(art->sample_num);
      if (art->loop_point != 0)
        rgn->SetLoopInfo(1, art->loop_point, sampcoll->samples[rgn->sampNum]->dataLength - art->loop_point);

      PSXConvADSR<AkaoRgn>(rgn, art->ADSR1, art->ADSR2, false);
      if (instr->bDrumKit)
        rgn->unityKey = art->unityKey + rgn->keyLow - rgn->drumRelUnityKey;
      else
        rgn->unityKey = art->unityKey;

      rgn->fineTune = art->fineTune;
    }
  }


  return true;
}

void AkaoColl::PreSynthFileCreation() {
  if (!dynamic_cast<AkaoSeq*>(seq)->bUsesIndividualArts)    //only do this if the 0xA1 event is actually used
    return;

  auto *instrSet = dynamic_cast<AkaoInstrSet *>(instrsets[0]);

  auto *sampcoll = dynamic_cast<AkaoSampColl *>(sampcolls[0]);
  const auto numArts = static_cast<uint32_t>(sampcoll->akArts.size());
  numAddedInstrs = numArts;

  for (uint32_t i = 0; i < numAddedInstrs; i++) {
    AkaoArt *art = &sampcoll->akArts[i];
    auto *newInstr = new AkaoInstr(instrSet, 0, 0, 0, sampcoll->starting_art_id + i);

    auto *rgn = new AkaoRgn(newInstr, 0, 0);

    rgn->SetSampNum(art->sample_num);
    if (art->loop_point != 0)
      rgn->SetLoopInfo(1, art->loop_point, sampcoll->samples[rgn->sampNum]->dataLength - art->loop_point);

    PSXConvADSR<AkaoRgn>(rgn, art->ADSR1, art->ADSR2, false);

    rgn->unityKey = art->unityKey;
    rgn->fineTune = art->fineTune;

    newInstr->aRgns.push_back(rgn);

    instrSet->aInstrs.push_back(newInstr);
  }
}

void AkaoColl::PostSynthFileCreation() {
  //if the 0xA1 event isn't used in the sequence, then we didn't modify the instrset
  //so skip this
  if (!dynamic_cast<AkaoSeq*>(seq)->bUsesIndividualArts)
    return;

  auto *instrSet = dynamic_cast<AkaoInstrSet *>(instrsets[0]);
  for (size_t i = 0; i < numAddedInstrs; i++) {
    delete instrSet->aInstrs.back();
    instrSet->aInstrs.pop_back();
  }
}
