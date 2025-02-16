#include "pch.h"
#include "SeqEvent.h"
#include "SeqTrack.h"

//  ********
//  SeqEvent
//  ********

//DECLARE_MENU(SeqEvent)

SeqEvent::SeqEvent(SeqTrack *pTrack,
                   uint32_t offset,
                   uint32_t length,
                   const std::wstring &_name,
                   uint8_t _color,
                   Icon _icon,
                   const std::wstring &_desc)
    : VGMItem(pTrack->parentSeq, offset, length, _name, _color), parentTrack(pTrack),icon(_icon), desc(_desc) {
}

// ***************
// DurNoteSeqEvent
// ***************

DurNoteSeqEvent::DurNoteSeqEvent(SeqTrack *pTrack,
                                 uint8_t absoluteKey,
                                 uint8_t velocity,
                                 uint32_t duration,
                                 uint32_t offset,
                                 uint32_t length,
                                 const std::wstring &_name)
    : SeqEvent(pTrack, offset, length, _name, CLR_DURNOTE), absKey(absoluteKey), vel(velocity), dur(duration) { }


// ************
// NoteOnSeqEvent
// ************

NoteOnSeqEvent::NoteOnSeqEvent(SeqTrack *pTrack,
                               uint8_t absoluteKey,
                               uint8_t velocity,
                               uint32_t offset,
                               uint32_t length,
                               const std::wstring &_name)
    : SeqEvent(pTrack, offset, length, _name, CLR_NOTEON), absKey(absoluteKey), vel(velocity) { }



// ************
// NoteOffSeqEvent
// ************

NoteOffSeqEvent::NoteOffSeqEvent(SeqTrack *pTrack,
                                 uint8_t absoluteKey,
                                 uint32_t offset,
                                 uint32_t length,
                                 const std::wstring &_name)
    : SeqEvent(pTrack, offset, length, _name, CLR_NOTEOFF), absKey(absoluteKey) { }

// ************
// RestSeqEvent
// ************

RestSeqEvent::RestSeqEvent(SeqTrack *pTrack,
                           uint32_t duration,
                           uint32_t offset,
                           uint32_t length,
                           const std::wstring &_name)
    : SeqEvent(pTrack, offset, length, _name, CLR_REST), dur(duration) { }

// *****************
// SetOctaveSeqEvent
// *****************

SetOctaveSeqEvent::SetOctaveSeqEvent(SeqTrack *pTrack,
                                     uint8_t theOctave,
                                     uint32_t offset,
                                     uint32_t length,
                                     const std::wstring &_name)
    : SeqEvent(pTrack, offset, length, _name, CLR_CHANGESTATE), octave(theOctave) { }

// ***********
// VolSeqEvent
// ***********

VolSeqEvent::VolSeqEvent(SeqTrack *pTrack, uint8_t volume, uint32_t offset, uint32_t length, const std::wstring &_name)
    : SeqEvent(pTrack, offset, length, _name, CLR_VOLUME), vol(volume) { }

// ****************
// VolSlideSeqEvent
// ****************

VolSlideSeqEvent::VolSlideSeqEvent(SeqTrack *pTrack,
                                   uint8_t targetVolume,
                                   uint32_t duration,
                                   uint32_t offset,
                                   uint32_t length,
                                   const std::wstring &_name)
    : SeqEvent(pTrack, offset, length, _name, CLR_VOLUME), targVol(targetVolume), dur(duration) { }

// ***********
// MastVolSeqEvent
// ***********

MastVolSeqEvent::MastVolSeqEvent(SeqTrack *pTrack,
                                 uint8_t volume,
                                 uint32_t offset,
                                 uint32_t length,
                                 const std::wstring &_name)
    : SeqEvent(pTrack, offset, length, _name, CLR_VOLUME), vol(volume) { }

// ****************
// MastVolSlideSeqEvent
// ****************

MastVolSlideSeqEvent::MastVolSlideSeqEvent(SeqTrack *pTrack,
                                           uint8_t targetVolume,
                                           uint32_t duration,
                                           uint32_t offset,
                                           uint32_t length,
                                           const std::wstring &_name)
    : SeqEvent(pTrack, offset, length, _name, CLR_VOLUME), targVol(targetVolume), dur(duration) { }

// ******************
// ExpressionSeqEvent
// ******************

ExpressionSeqEvent::ExpressionSeqEvent(SeqTrack *pTrack,
                                       uint8_t theLevel,
                                       uint32_t offset,
                                       uint32_t length,
                                       const std::wstring &_name)
    : SeqEvent(pTrack, offset, length, _name, CLR_EXPRESSION), level(theLevel) { }

// ***********************
// ExpressionSlideSeqEvent
// ***********************

ExpressionSlideSeqEvent::ExpressionSlideSeqEvent(SeqTrack *pTrack,
                                                 uint8_t targetExpression,
                                                 uint32_t duration,
                                                 uint32_t offset,
                                                 uint32_t length,
                                                 const std::wstring &_name)
    : SeqEvent(pTrack, offset, length, _name, CLR_EXPRESSION), targExpr(targetExpression), dur(duration) { }



// ***********
// PanSeqEvent
// ***********

PanSeqEvent::PanSeqEvent(SeqTrack *pTrack, uint8_t thePan, uint32_t offset, uint32_t length, const std::wstring &_name)
    : SeqEvent(pTrack, offset, length, _name, CLR_PAN), pan(thePan) { }

// ****************
// PanSlideSeqEvent
// ****************

PanSlideSeqEvent::PanSlideSeqEvent(SeqTrack *pTrack,
                                   uint8_t targetPan,
                                   uint32_t duration,
                                   uint32_t offset,
                                   uint32_t length,
                                   const std::wstring &_name)
    : SeqEvent(pTrack, offset, length, _name, CLR_PAN), targPan(targetPan), dur(duration) { }

// **************
// ReverbSeqEvent
// **************

ReverbSeqEvent::ReverbSeqEvent(SeqTrack *pTrack,
                               uint8_t theReverb,
                               uint32_t offset,
                               uint32_t length,
                               const std::wstring &_name)
    : SeqEvent(pTrack, offset, length, _name, CLR_REVERB), reverb(theReverb) { }

// *****************
// PitchBendSeqEvent
// *****************

PitchBendSeqEvent::PitchBendSeqEvent(SeqTrack *pTrack,
                                     short thePitchBend,
                                     uint32_t offset,
                                     uint32_t length,
                                     const std::wstring &_name)
    : SeqEvent(pTrack, offset, length, _name, CLR_PITCHBEND), pitchbend(thePitchBend) { }

// **********************
// PitchBendRangeSeqEvent
// **********************

PitchBendRangeSeqEvent::PitchBendRangeSeqEvent(SeqTrack *pTrack, uint8_t _semitones, uint8_t _cents,
                                               uint32_t offset, uint32_t length, const std::wstring &_name)
    : SeqEvent(pTrack, offset, length, _name, CLR_PITCHBENDRANGE), semitones(_semitones), cents(_cents) { }

// ******************
// FineTuningSeqEvent
// ******************

FineTuningSeqEvent::FineTuningSeqEvent(SeqTrack *pTrack, double _cents,
                                       uint32_t offset, uint32_t length, const std::wstring &_name)
    : SeqEvent(pTrack, offset, length, _name, CLR_MISC), cents(_cents) { }

// ****************************
// ModulationDepthRangeSeqEvent
// ****************************

ModulationDepthRangeSeqEvent::ModulationDepthRangeSeqEvent(SeqTrack *pTrack, double _semitones,
                                                           uint32_t offset, uint32_t length, const std::wstring &_name)
    : SeqEvent(pTrack, offset, length, _name, CLR_MISC), semitones(_semitones) { }

// *****************
// TransposeSeqEvent
// *****************

TransposeSeqEvent::TransposeSeqEvent(SeqTrack *pTrack,
                                     int theTranspose,
                                     uint32_t offset,
                                     uint32_t length,
                                     const std::wstring &_name)
    : SeqEvent(pTrack, offset, length, _name, CLR_TRANSPOSE), transpose(theTranspose) { }

// ******************
// ModulationSeqEvent
// ******************

ModulationSeqEvent::ModulationSeqEvent(SeqTrack *pTrack,
                                       uint8_t theDepth,
                                       uint32_t offset,
                                       uint32_t length,
                                       const std::wstring &_name)
    : SeqEvent(pTrack, offset, length, _name, CLR_MODULATION), depth(theDepth) { }

// **************
// BreathSeqEvent
// **************

BreathSeqEvent::BreathSeqEvent(SeqTrack *pTrack,
                               uint8_t theDepth,
                               uint32_t offset,
                               uint32_t length,
                               const std::wstring &_name)
    : SeqEvent(pTrack, offset, length, _name, CLR_MODULATION), depth(theDepth) { }

// ***************
// SustainSeqEvent
// ***************

SustainSeqEvent::SustainSeqEvent(SeqTrack *pTrack,
                                 uint8_t theDepth,
                                 uint32_t offset,
                                 uint32_t length,
                                 const std::wstring &_name)
    : SeqEvent(pTrack, offset, length, _name, CLR_SUSTAIN), depth(theDepth) { }

// ******************
// PortamentoSeqEvent
// ******************

PortamentoSeqEvent::PortamentoSeqEvent(SeqTrack *pTrack,
                                       bool bPortamento,
                                       uint32_t offset,
                                       uint32_t length,
                                       const std::wstring &_name)
    : SeqEvent(pTrack, offset, length, _name, CLR_PORTAMENTO), bOn(bPortamento) { }

// **********************
// PortamentoTimeSeqEvent
// **********************

PortamentoTimeSeqEvent::PortamentoTimeSeqEvent(SeqTrack *pTrack,
                                               uint8_t theTime,
                                               uint32_t offset,
                                               uint32_t length,
                                               const std::wstring &_name)
    : SeqEvent(pTrack, offset, length, _name, CLR_PORTAMENTO), time(theTime) { }

// ******************
// ProgChangeSeqEvent
// ******************

ProgChangeSeqEvent::ProgChangeSeqEvent(SeqTrack *pTrack,
                                       uint32_t programNumber,
                                       uint32_t offset,
                                       uint32_t length,
                                       const std::wstring &_name)
    : SeqEvent(pTrack, offset, length, _name, CLR_PROGCHANGE), progNum(programNumber) { }

// *************
// TempoSeqEvent
// *************

TempoSeqEvent::TempoSeqEvent(SeqTrack *pTrack,
                             double beatsperminute,
                             uint32_t offset,
                             uint32_t length,
                             const std::wstring &_name)
    : SeqEvent(pTrack, offset, length, _name, CLR_TEMPO), bpm(beatsperminute) { }

// ******************
// TempoSlideSeqEvent
// ******************

TempoSlideSeqEvent::TempoSlideSeqEvent(SeqTrack *pTrack,
                                       double targBPM,
                                       uint32_t duration,
                                       uint32_t offset,
                                       uint32_t length,
                                       const std::wstring &_name)
    : SeqEvent(pTrack, offset, length, _name, CLR_TEMPO), targbpm(targBPM), dur(duration) { }

// ***************
// TimeSigSeqEvent
// ***************

TimeSigSeqEvent::TimeSigSeqEvent(SeqTrack *pTrack,
                                 uint8_t numerator,
                                 uint8_t denominator,
                                 uint8_t theTicksPerQuarter,
                                 uint32_t offset,
                                 uint32_t length,
                                 const std::wstring &_name)
    : SeqEvent(pTrack, offset, length, _name, CLR_TIMESIG), numer(numerator), denom(denominator),
      ticksPerQuarter(theTicksPerQuarter) { }


