#include "pch.h"
#include "VGMSeqNoTrks.h"

#include <utility>
#include "SeqEvent.h"
#include "Root.h"

using namespace std;

VGMSeqNoTrks::VGMSeqNoTrks(const string &_format, RawFile *file, uint32_t offset, wstring _name)
    : VGMSeq(_format, file, offset, 0, std::move(_name)),
      SeqTrack(this) {
  ResetVars();
  VGMSeq::AddContainer<SeqEvent>(aEvents);
}

VGMSeqNoTrks::~VGMSeqNoTrks() = default;

void VGMSeqNoTrks::ResetVars() {
  midiTracks.clear();        //no need to delete the contents, that happens when the midi is deleted
  TryExpandMidiTracks(nNumTracks);

  channel = 0;
  SetCurTrack(channel);

  VGMSeq::ResetVars();
  SeqTrack::ResetVars();
}

//LoadMain() - loads all sequence data into the class
bool VGMSeqNoTrks::LoadMain() {
  this->SeqTrack::readMode = this->VGMSeq::readMode = READMODE_ADD_TO_UI;
  if (!GetHeaderInfo())
    return false;

  if (!LoadEvents())
    return false;

  if (length() == 0) {
    VGMSeq::SetGuessedLength();
//		length() = (aEvents.back()->dwOffset + aEvents.back()->unLength) - offset();			//length == to the end of the last event
  }

  return true;
}

bool VGMSeqNoTrks::LoadEvents(long stopTime) {
  ResetVars();
  if (bAlwaysWriteInitialTempo)
    AddTempoBPMNoItem(initialTempoBPM);
  if (bAlwaysWriteInitialVol)
    for (int i = 0; i < 16; i++) {
      channel = i;
      AddVolNoItem(initialVol);
    }
  if (bAlwaysWriteInitialExpression)
    for (int i = 0; i < 16; i++) {
      channel = i;
      AddExpressionNoItem(initialExpression);
    }
  if (bAlwaysWriteInitialReverb)
    for (int i = 0; i < 16; i++) {
      channel = i;
      AddReverbNoItem(initialReverb);
    }
  if (bAlwaysWriteInitialPitchBendRange)
    for (int i = 0; i < 16; i++) {
      channel = i;
      AddPitchBendRangeNoItem(initialPitchBendRangeSemiTones, initialPitchBendRangeCents);
    }

  bInLoop = false;
  curOffset = eventsOffset();    //start at beginning of track
  while (curOffset < rawfile->size()) {
    if (GetTime() >= stopTime) {
      break;
    }

    if (!ReadEvent()) {
      break;
    }
  }
  return true;
}


MidiFile *VGMSeqNoTrks::ConvertToMidi() {
  this->SeqTrack::readMode = this->VGMSeq::readMode = READMODE_FIND_DELTA_LENGTH;

  if (!LoadEvents())
    return nullptr;
  if (!PostLoad())
    return nullptr;

  long stopTime = -1;
  stopTime = deltaLength;

  MidiFile *newmidi = new MidiFile(this);
  this->midi = newmidi;
  this->SeqTrack::readMode = this->VGMSeq::readMode = READMODE_CONVERT_TO_MIDI;
  if (!LoadEvents(stopTime)) {
    delete midi;
    this->midi = nullptr;
    return nullptr;
  }
  if (!PostLoad()) {
    delete midi;
    this->midi = nullptr;
    return nullptr;
  }
  this->midi = nullptr;
  return newmidi;
}

MidiTrack *VGMSeqNoTrks::GetFirstMidiTrack() {
  if (!midiTracks.empty()) {
    return midiTracks[0];
  }
  else {
    return pMidiTrack;
  }
}

// checks whether or not we have already created the given number of MidiTracks.  If not, it appends the extra tracks.
// doesn't ever need to be called directly by format code, since SetCurMidiTrack does so automatically.
void VGMSeqNoTrks::TryExpandMidiTracks(uint32_t numTracks) {
  if (VGMSeq::readMode != READMODE_CONVERT_TO_MIDI)
    return;
  if (midiTracks.size() < numTracks) {
    size_t initialTrackSize = midiTracks.size();
    for (size_t i = 0; i < numTracks - initialTrackSize; i++)
      midiTracks.push_back(midi->AddTrack());
  }
}

void VGMSeqNoTrks::SetCurTrack(uint32_t trackNum) {
  if (VGMSeq::readMode != READMODE_CONVERT_TO_MIDI)
    return;

  TryExpandMidiTracks(trackNum + 1);
  pMidiTrack = midiTracks[trackNum];
}


void VGMSeqNoTrks::AddTime(uint32_t delta) {
  VGMSeq::time += delta;
  if (VGMSeq::readMode == READMODE_CONVERT_TO_MIDI) {
    for (auto & midiTrack : midiTracks)
      midiTrack->AddDelta(delta);
  }
}
