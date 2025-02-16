#include "pch.h"
#include "common.h"
#include "VGMSeq.h"

#include <utility>
#include "SeqTrack.h"
#include "SeqEvent.h"
#include "SeqSlider.h"
#include "Options.h"
#include "Root.h"


DECLARE_MENU(VGMSeq)

using namespace std;

VGMSeq::VGMSeq(const string &_format, RawFile *file, uint32_t offset, uint32_t length, wstring _name)
    : VGMFile(FILETYPE_SEQ, _format, file, offset, length, std::move(_name)),
      nNumTracks(0),
      readMode(READMODE_ADD_TO_UI),
      midi(nullptr),
      time(0),
      bMonophonicTracks(false),
      bUseLinearAmplitudeScale(false),
      bUseLinearPanAmplitudeScale(false),
      bAlwaysWriteInitialTempo(false),
      bAlwaysWriteInitialVol(false),
      bAlwaysWriteInitialExpression(false),
      bAlwaysWriteInitialReverb(false),
      bAlwaysWriteInitialPitchBendRange(false),
      bAlwaysWriteInitialMono(false),
      bAllowDiscontinuousTrackData(false),
      bLoadTickByTick(false),
      initialVol(100),                    //GM standard (dls1 spec p16)
      initialExpression(127),            //''
      initialReverb(40),                //GM standard
      initialPitchBendRangeSemiTones(2), //GM standard.  Means +/- 2 semitones (4 total range)
      initialPitchBendRangeCents(0),
      initialTempoBPM(120),
      bReverb(false) {
  AddContainer<SeqTrack>(aTracks);
}

VGMSeq::~VGMSeq() {
  DeleteVect<SeqTrack>(aTracks);
  DeleteVect<ISeqSlider>(aSliders);
}

bool VGMSeq::Load() {
  if (!LoadMain())
    return false;

  //LoadLocalData();
  //UseLocalData();
  pRoot->AddVGMFile(this);
  return true;
}

MidiFile *VGMSeq::ConvertToMidi() {
  size_t numTracks = aTracks.size();

  if (!LoadTracks(READMODE_FIND_DELTA_LENGTH))
    return nullptr;

  // Find the greatest length of all tracks to use as stop point for every track
  long stopTime = -1;
  for (size_t i = 0; i < numTracks; i++)
    stopTime = max(stopTime, aTracks[i]->deltaLength);

  MidiFile *newmidi = new MidiFile(this);
  this->midi = newmidi;
  if (!LoadTracks(READMODE_CONVERT_TO_MIDI, stopTime)) {
    delete midi;
    this->midi = nullptr;
    return nullptr;
  }
  this->midi = nullptr;
  return newmidi;
}

MidiTrack *VGMSeq::GetFirstMidiTrack() {
  MidiTrack *pFirstMidiTrack = nullptr;
  if (!aTracks.empty())
    pFirstMidiTrack = aTracks[0]->pMidiTrack;
  return pFirstMidiTrack;
}

//Load() - Function to load all the sequence data into the class
bool VGMSeq::LoadMain() {
  readMode = READMODE_ADD_TO_UI;

  if (!GetHeaderInfo())
    return false;
  if (!GetTrackPointers())
    return false;
  nNumTracks = aTracks.size();
  if (nNumTracks == 0)
    return false;

  if (!LoadTracks(readMode))
    return false;

  return true;
}

bool VGMSeq::PostLoad() {
  if (readMode == READMODE_ADD_TO_UI) {
    std::sort(aInstrumentsUsed.begin(), aInstrumentsUsed.end());

    for (auto & aTrack : aTracks) {
      std::sort(aTrack->aEvents.begin(), aTrack->aEvents.end(), ItemPtrOffsetCmp());
    }
  }
  else if (readMode == READMODE_CONVERT_TO_MIDI) {
    midi->Sort();
  }

  return true;
}

bool VGMSeq::LoadTracks(ReadMode _readMode, long stopTime) {
  bool succeeded = true;

  // set read mode
  this->readMode = _readMode;
  for (uint32_t trackNum = 0; trackNum < nNumTracks; trackNum++) {
    aTracks[trackNum]->readMode = _readMode;
  }

  // reset variables
  ResetVars();
  for (uint32_t trackNum = 0; trackNum < nNumTracks; trackNum++) {
    if (!aTracks[trackNum]->LoadTrackInit(trackNum, nullptr))
      return false;
  }

  LoadTracksMain(stopTime);
  if (_readMode == READMODE_ADD_TO_UI) {
    SetGuessedLength();
    if (unLength == 0) {
      return false;
    }
  }

  if (!PostLoad()) {
    succeeded = false;
  }

  return succeeded;
}

void VGMSeq::LoadTracksMain(long stopTime) {
  // determine the stop offsets
  uint32_t *aStopOffset = new uint32_t[nNumTracks];
  for (uint32_t trackNum = 0; trackNum < nNumTracks; trackNum++) {
    if (readMode == READMODE_ADD_TO_UI) {
      aStopOffset[trackNum] = GetEndOffset();
      if (unLength != 0) {
        aStopOffset[trackNum] = dwOffset + unLength;
      }
      else {
        if (!bAllowDiscontinuousTrackData) {
          // set length from the next track by offset
          for (uint32_t j = 0; j < nNumTracks; j++) {
            if (aTracks[j]->dwOffset > aTracks[trackNum]->dwOffset &&
                aTracks[j]->dwOffset < aStopOffset[trackNum]) {
              aStopOffset[trackNum] = aTracks[j]->dwOffset;
            }
          }
        }
      }
    }
    else {
      aStopOffset[trackNum] = aTracks[trackNum]->dwOffset + aTracks[trackNum]->unLength;
    }
  }

  // load all tracks
  if (bLoadTickByTick) {
    while (HasActiveTracks()) {
      // check time limit
      if (time >= stopTime) {
        if (readMode == READMODE_ADD_TO_UI) {
          wstring itemName = *this->GetName() + L" - Abort loading tracks by time limit.";
          pRoot->AddLogItem(new LogItem(itemName.c_str(), LOG_LEVEL_WARN, L"VGMSeq"));
        }

        InactivateAllTracks();
        break;
      }

      // process tracks
      for (uint32_t trackNum = 0; trackNum < nNumTracks; trackNum++) {
        if (!aTracks[trackNum]->active)
          continue;

        // tick
        aTracks[trackNum]->LoadTrackMainLoop(aStopOffset[trackNum], stopTime);
      }

      // process sliders
      auto itrSlider = aSliders.begin();
      while (itrSlider != aSliders.end()) {
        auto itrNextSlider = itrSlider + 1;

        ISeqSlider *slider = *itrSlider;
        if (slider->isStarted(time)) {
          if (slider->isActive(time)) {
            slider->write(time);
          }
          else {
            itrNextSlider = aSliders.erase(itrSlider);
          }
        }

        itrSlider = itrNextSlider;
      }

      time++;

      if (readMode == READMODE_CONVERT_TO_MIDI) {
        for (uint32_t trackNum = 0; trackNum < nNumTracks; trackNum++) {
          if (aTracks[trackNum]->pMidiTrack != nullptr) {
            aTracks[trackNum]->pMidiTrack->SetDelta(time);
          }
        }
      }

      // check loop count
      int requiredLoops = (readMode == READMODE_ADD_TO_UI) ? 1 : ConversionOptions::GetNumSequenceLoops();
      if (GetForeverLoops() >= requiredLoops) {
        InactivateAllTracks();
        break;
      }
    }
  }
  else {
    uint32_t initialTime = time; // preserve current time for multi section sequence

    // load track by track
    for (uint32_t trackNum = 0; trackNum < nNumTracks; trackNum++) {
      time = initialTime;

      aTracks[trackNum]->LoadTrackMainLoop(aStopOffset[trackNum], stopTime);
      aTracks[trackNum]->active = false;
    }
  }
  delete[] aStopOffset;
}

bool VGMSeq::HasActiveTracks() {
  for (uint32_t trackNum = 0; trackNum < nNumTracks; trackNum++) {
    if (aTracks[trackNum]->active)
      return true;
  }
  return false;
}

void VGMSeq::InactivateAllTracks() {
  for (uint32_t trackNum = 0; trackNum < nNumTracks; trackNum++) {
    aTracks[trackNum]->active = false;
  }
}

int VGMSeq::GetForeverLoops() {
  if (nNumTracks == 0)
    return 0;

  int foreverLoops = INT_MAX;
  for (uint32_t trackNum = 0; trackNum < nNumTracks; trackNum++) {
    if (!aTracks[trackNum]->active)
      continue;

    if (foreverLoops > aTracks[trackNum]->foreverLoops)
      foreverLoops = aTracks[trackNum]->foreverLoops;
  }
  return (foreverLoops != INT_MAX) ? foreverLoops : 0;
}

bool VGMSeq::GetHeaderInfo() {
  return true;
}


//GetTrackPointers() should contain logic for parsing track pointers
// and instantiating/adding each track in the sequence
bool VGMSeq::GetTrackPointers() {
  return true;
}

void VGMSeq::ResetVars() {
  time = 0;
  tempoBPM = initialTempoBPM;

  DeleteVect<ISeqSlider>(aSliders);

  if (readMode == READMODE_ADD_TO_UI) {
    aInstrumentsUsed.clear();
  }
}

void VGMSeq::SetPPQN(uint16_t _ppqn) {
  this->ppqn = _ppqn;
  if (readMode == READMODE_CONVERT_TO_MIDI)
    midi->SetPPQN(_ppqn);
}

uint16_t VGMSeq::GetPPQN() const {
  return this->ppqn;
  //return midi->GetPPQN();
}

void VGMSeq::AddInstrumentRef(uint32_t progNum) {
  if (std::find(aInstrumentsUsed.begin(), aInstrumentsUsed.end(), progNum) == aInstrumentsUsed.end()) {
    aInstrumentsUsed.push_back(progNum);
  }
}

bool VGMSeq::OnSaveAsMidi() {
  wstring filepath = pRoot->UI_GetSaveFilePath(ConvertToSafeFileName(name), L"mid");
  if (filepath.length() != 0)
    return SaveAsMidi(filepath);
  return false;
}


bool VGMSeq::SaveAsMidi(const std::wstring &filepath) {
  MidiFile *_midi = this->ConvertToMidi();
  if (!_midi)
    return false;
  bool result = _midi->SaveMidiFile(filepath);
  delete _midi;
  return result;
}
