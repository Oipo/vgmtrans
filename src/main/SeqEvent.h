#pragma once
#include "VGMItem.h"
#include "Menu.h"
#include "MidiFile.h"

#define DESCRIPTION(_str_)                                   \
    virtual std::wstring GetDescription()                    \
    {                                                        \
        std::wostringstream    _desc;                         \
        _desc << name << L" -  " << _str_;                    \
        return _desc.str();                                   \
    }


class SeqTrack;

enum EventType {
  EVENTTYPE_UNDEFINED,
  EVENTTYPE_NOTEON,
  EVENTTYPE_NOTEOFF,
  EVENTTYPE_DURNOTE,
  EVENTTYPE_REST,
  EVENTTYPE_EXPRESSION,
  EVENTTYPE_EXPRESSIONSLIDE,
  EVENTTYPE_VOLUME,
  EVENTTYPE_VOLUMESLIDE,
  EVENTTYPE_PAN,
  EVENTTYPE_REVERB,
  EVENTTYPE_PROGCHANGE,
  EVENTTYPE_PITCHBEND,
  EVENTTYPE_PITCHBENDRANGE,
  EVENTTYPE_FINETUNING,
  EVENTTYPE_TRANSPOSE,
  EVENTTYPE_TEMPO,
  EVENTTYPE_TIMESIG,
  EVENTTYPE_MODULATION,
  EVENTTYPE_BREATH,
  EVENTTYPE_SUSTAIN,
  EVENTTYPE_PORTAMENTO,
  EVENTTYPE_PORTAMENTOTIME,
  EVENTTYPE_MARKER,
  EVENTTYPE_TRACKEND,
  EVENTTYPE_LOOPFOREVER
};

class SeqEvent:
    public VGMItem {
 public:
  SeqEvent(SeqTrack *pTrack,
           uint32_t offset = 0,
           uint32_t length = 0,
           const std::wstring &_name = L"",
           uint8_t color = 0,
           Icon icon = ICON_BINARY,
           const std::wstring &desc = L"");
  ~SeqEvent() override = default;
  std::wstring GetDescription() override {
    return desc.empty() ? std::wstring(name) : (std::wstring(name) + L" - " + desc);
  }
  [[nodiscard]] ItemType GetType() const override { return ITEMTYPE_SEQEVENT; }
  virtual EventType GetEventType() { return EVENTTYPE_UNDEFINED; }
  Icon GetIcon() override { return icon; }

 public:
  uint8_t channel;
  SeqTrack *parentTrack;
 private:
  Icon icon;
  std::wstring desc;
};


//  ***************
//  DurNoteSeqEvent
//  ***************

class DurNoteSeqEvent:
    public SeqEvent {
 public:
  DurNoteSeqEvent(SeqTrack *pTrack,
                  uint8_t absoluteKey,
                  uint8_t velocity,
                  uint32_t duration,
                  uint32_t offset = 0,
                  uint32_t length = 0,
                  const std::wstring &_name = L"");
  ~DurNoteSeqEvent() override = default;
  EventType GetEventType() override { return EVENTTYPE_DURNOTE; }
  Icon GetIcon() override { return ICON_NOTE; }
  DESCRIPTION(
      L"Abs Key: " << absKey << " (" << MidiEvent::GetNoteName(absKey) << ") " << L"  Velocity: " << vel
          << L"  Duration: " << dur)
 public:
  uint8_t absKey;
  uint8_t vel;
  uint32_t dur;
};

//  **************
//  NoteOnSeqEvent
//  ***************

class NoteOnSeqEvent:
    public SeqEvent {
 public:
  NoteOnSeqEvent(SeqTrack *pTrack,
                 uint8_t absoluteKey,
                 uint8_t velocity,
                 uint32_t offset = 0,
                 uint32_t length = 0,
                 const std::wstring &_name = L"");
  ~NoteOnSeqEvent() override = default;
  EventType GetEventType() override { return EVENTTYPE_NOTEON; }
  Icon GetIcon() override { return ICON_NOTE; }
  DESCRIPTION(
      L"Abs Key: " << absKey << " (" << MidiEvent::GetNoteName(absKey) << ") " << L"  Velocity: " << vel)
 public:
  uint8_t absKey;
  uint8_t vel;
};

//  ***************
//  NoteOffSeqEvent
//  ***************

class NoteOffSeqEvent:
    public SeqEvent {
 public:
  NoteOffSeqEvent
      (SeqTrack *pTrack, uint8_t absoluteKey, uint32_t offset = 0, uint32_t length = 0, const std::wstring &_name = L"");
  ~NoteOffSeqEvent() override { }
  EventType GetEventType() override { return EVENTTYPE_NOTEOFF; }
  Icon GetIcon() override { return ICON_NOTE; }
  DESCRIPTION(L"Abs Key: " << absKey << " (" << MidiEvent::GetNoteName(absKey) << ") ")
 public:
  uint8_t absKey;
};

//  ************
//  RestSeqEvent
//  ************

class RestSeqEvent:
    public SeqEvent {
 public:
  RestSeqEvent
      (SeqTrack *pTrack, uint32_t duration, uint32_t offset = 0, uint32_t length = 0, const std::wstring &_name = L"");
  ~RestSeqEvent() override = default;
  EventType GetEventType() override { return EVENTTYPE_REST; }
  Icon GetIcon() override { return ICON_REST; }
  DESCRIPTION(L"Duration: " << dur)

 public:
  uint32_t dur;
};

//  *****************
//  SetOctaveSeqEvent
//  *****************

class SetOctaveSeqEvent:
    public SeqEvent {
 public:
  SetOctaveSeqEvent
      (SeqTrack *pTrack, uint8_t octave, uint32_t offset = 0, uint32_t length = 0, const std::wstring &_name = L"");
  ~SetOctaveSeqEvent() override = default;
  DESCRIPTION(L"Octave: " << octave)
 public:
  uint8_t octave;
};

//  ***********
//  VolSeqEvent
//  ***********

class VolSeqEvent:
    public SeqEvent {
 public:
  VolSeqEvent
      (SeqTrack *pTrack, uint8_t volume, uint32_t offset = 0, uint32_t length = 0, const std::wstring &_name = L"");
  ~VolSeqEvent() override = default;
  EventType GetEventType() override { return EVENTTYPE_VOLUME; }
  DESCRIPTION(L"Volume: " << vol)

 public:
  uint8_t vol;
};

//  ****************
//  VolSlideSeqEvent
//  ****************

class VolSlideSeqEvent:
    public SeqEvent {
 public:
  VolSlideSeqEvent(SeqTrack *pTrack,
                   uint8_t targetVolume,
                   uint32_t duration,
                   uint32_t offset = 0,
                   uint32_t length = 0,
                   const std::wstring &_name = L"");
  ~VolSlideSeqEvent() override = default;
  EventType GetEventType() override { return EVENTTYPE_VOLUMESLIDE; }
  DESCRIPTION(L"Target Volume: " << targVol << L"  Duration: " << dur)

 public:
  uint8_t targVol;
  uint32_t dur;
};


//  ***********
//  MastVolSeqEvent
//  ***********

class MastVolSeqEvent:
    public SeqEvent {
 public:
  MastVolSeqEvent
      (SeqTrack *pTrack, uint8_t volume, uint32_t offset = 0, uint32_t length = 0, const std::wstring &_name = L"");
  ~MastVolSeqEvent() override = default;
  DESCRIPTION(L"Master Volume: " << vol)

 public:
  uint8_t vol;
};

//  ****************
//  MastVolSlideSeqEvent
//  ****************

class MastVolSlideSeqEvent:
    public SeqEvent {
 public:
  MastVolSlideSeqEvent(SeqTrack *pTrack,
                       uint8_t targetVolume,
                       uint32_t duration,
                       uint32_t offset = 0,
                       uint32_t length = 0,
                       const std::wstring &_name = L"");
  ~MastVolSlideSeqEvent() override = default;
  DESCRIPTION(L"Target Volume: " << targVol << L"  Duration: " << dur)

 public:
  uint8_t targVol;
  uint32_t dur;
};

//  ******************
//  ExpressionSeqEvent
//  ******************

class ExpressionSeqEvent:
    public SeqEvent {
 public:
  ExpressionSeqEvent
      (SeqTrack *pTrack, uint8_t level, uint32_t offset = 0, uint32_t length = 0, const std::wstring &_name = L"");
  ~ExpressionSeqEvent() override = default;
  EventType GetEventType() override { return EVENTTYPE_EXPRESSION; }
  DESCRIPTION(L"Expression: " << level)

 public:
  uint8_t level;
};

//  ***********************
//  ExpressionSlideSeqEvent
//  ***********************

class ExpressionSlideSeqEvent:
    public SeqEvent {
 public:
  ExpressionSlideSeqEvent(SeqTrack *pTrack,
                          uint8_t targetExpression,
                          uint32_t duration,
                          uint32_t offset = 0,
                          uint32_t length = 0,
                          const std::wstring &_name = L"");
  ~ExpressionSlideSeqEvent() override = default;
  EventType GetEventType() override { return EVENTTYPE_EXPRESSIONSLIDE; }
  DESCRIPTION(L"Target Expression: " << targExpr << L"  Duration: " << dur)

 public:
  uint8_t targExpr;
  uint32_t dur;
};


//  ***********
//  PanSeqEvent
//  ***********

class PanSeqEvent:
    public SeqEvent {
 public:
  PanSeqEvent(SeqTrack *pTrack, uint8_t pan, uint32_t offset = 0, uint32_t length = 0, const std::wstring &_name = L"");
  ~PanSeqEvent() override = default;
  EventType GetEventType() override { return EVENTTYPE_PAN; }
  DESCRIPTION(L"Pan: " << pan)

 public:
  uint8_t pan;
};

//  ****************
//  PanSlideSeqEvent
//  ****************

class PanSlideSeqEvent:
    public SeqEvent {
 public:
  PanSlideSeqEvent(SeqTrack *pTrack,
                   uint8_t targetPan,
                   uint32_t duration,
                   uint32_t offset = 0,
                   uint32_t length = 0,
                   const std::wstring &_name = L"");
  ~PanSlideSeqEvent() override = default;
  DESCRIPTION(L"Target Pan: " << targPan << L"  Duration: " << dur)

 public:
  uint8_t targPan;
  uint32_t dur;
};


//  **************
//  ReverbSeqEvent
//  **************

class ReverbSeqEvent:
    public SeqEvent {
 public:
  ReverbSeqEvent
      (SeqTrack *pTrack, uint8_t reverb, uint32_t offset = 0, uint32_t length = 0, const std::wstring &_name = L"");
  ~ReverbSeqEvent() override = default;
  EventType GetEventType() override { return EVENTTYPE_REVERB; }
  DESCRIPTION(L"Reverb: " << reverb)

 public:
  uint8_t reverb;
};


//  *****************
//  PitchBendSeqEvent
//  *****************

class PitchBendSeqEvent:
    public SeqEvent {
 public:
  PitchBendSeqEvent
      (SeqTrack *pTrack, short thePitchBend, uint32_t offset = 0, uint32_t length = 0, const std::wstring &_name = L"");
  EventType GetEventType() override { return EVENTTYPE_PITCHBEND; }
  DESCRIPTION(L"Pitch Bend: " << pitchbend)

 public:
  short pitchbend;
};


//  **********************
//  PitchBendRangeSeqEvent
//  **********************

class PitchBendRangeSeqEvent:
    public SeqEvent {
 public:
  PitchBendRangeSeqEvent(SeqTrack *pTrack, uint8_t semiTones, uint8_t cents,
                         uint32_t offset = 0, uint32_t length = 0, const std::wstring &_name = L"");
  EventType GetEventType() override { return EVENTTYPE_PITCHBENDRANGE; }
  DESCRIPTION(L"Pitch Bend Range: " << semitones << L" semitones, " << cents << L" cents")

 public:
  uint8_t semitones;
  uint8_t cents;
};

//  ******************
//  FineTuningSeqEvent
//  ******************

class FineTuningSeqEvent:
    public SeqEvent {
 public:
  FineTuningSeqEvent(SeqTrack *pTrack, double cents,
                     uint32_t offset = 0, uint32_t length = 0, const std::wstring &_name = L"");
  EventType GetEventType() override { return EVENTTYPE_PITCHBENDRANGE; }
  DESCRIPTION(L"Fine Tuning: " << cents << L" cents")

 public:
  double cents;
};

//  ****************************
//  ModulationDepthRangeSeqEvent
//  ****************************

class ModulationDepthRangeSeqEvent:
    public SeqEvent {
 public:
  ModulationDepthRangeSeqEvent(SeqTrack *pTrack, double semitones,
                               uint32_t offset = 0, uint32_t length = 0, const std::wstring &_name = L"");
  EventType GetEventType() override { return EVENTTYPE_PITCHBENDRANGE; }
  DESCRIPTION(L"Modulation Depth Range: " << (semitones * 100.0) << L" cents")

 public:
  double semitones;
};

//  *****************
//  TransposeSeqEvent
//  *****************

class TransposeSeqEvent:
    public SeqEvent {
 public:
  TransposeSeqEvent
      (SeqTrack *pTrack, int theTranspose, uint32_t offset = 0, uint32_t length = 0, const std::wstring &_name = L"");
  EventType GetEventType() override { return EVENTTYPE_TRANSPOSE; }
  DESCRIPTION(L"Transpose: " << transpose)

 public:
  int transpose;
};


//  ******************
//  ModulationSeqEvent
//  ******************

class ModulationSeqEvent:
    public SeqEvent {
 public:
  ModulationSeqEvent
      (SeqTrack *pTrack, uint8_t theDepth, uint32_t offset = 0, uint32_t length = 0, const std::wstring &_name = L"");
  ~ModulationSeqEvent() override = default;
  EventType GetEventType() override { return EVENTTYPE_MODULATION; }
  DESCRIPTION(L"Depth: " << depth)

 public:
  uint8_t depth;
};

//  **************
//  BreathSeqEvent
//  **************

class BreathSeqEvent:
    public SeqEvent {
 public:
  BreathSeqEvent
      (SeqTrack *pTrack, uint8_t theDepth, uint32_t offset = 0, uint32_t length = 0, const std::wstring &_name = L"");
  ~BreathSeqEvent() override = default;
  EventType GetEventType() override { return EVENTTYPE_BREATH; }
  DESCRIPTION(L"Breath: " << depth)

 public:
  uint8_t depth;
};

//  ****************
//  SustainSeqEvent
//  ****************

class SustainSeqEvent:
    public SeqEvent {
 public:
  SustainSeqEvent
      (SeqTrack *pTrack, uint8_t theDepth, uint32_t offset = 0, uint32_t length = 0, const std::wstring &_name = L"");
  ~SustainSeqEvent() override = default;
  EventType GetEventType() override { return EVENTTYPE_SUSTAIN; }
  DESCRIPTION(L"Sustain Pedal: " << depth);

 public:
  uint8_t depth;
};

//  ******************
//  PortamentoSeqEvent
//  ******************

class PortamentoSeqEvent:
    public SeqEvent {
 public:
  PortamentoSeqEvent
      (SeqTrack *pTrack, bool bPortamento, uint32_t offset = 0, uint32_t length = 0, const std::wstring &_name = L"");
  ~PortamentoSeqEvent() override = default;
  EventType GetEventType() override { return EVENTTYPE_PORTAMENTO; }
  DESCRIPTION(L"Portamento: " << ((bOn) ? L"On" : L"Off"))

 public:
  bool bOn;
};

//  **********************
//  PortamentoTimeSeqEvent
//  **********************

class PortamentoTimeSeqEvent:
    public SeqEvent {
 public:
  PortamentoTimeSeqEvent
      (SeqTrack *pTrack, uint8_t time, uint32_t offset = 0, uint32_t length = 0, const std::wstring &_name = L"");
  ~PortamentoTimeSeqEvent() override = default;
  EventType GetEventType() override { return EVENTTYPE_PORTAMENTOTIME; }
  DESCRIPTION(L"Portamento Time: " << time)

 public:
  uint8_t time;
};


//  ******************
//  ProgChangeSeqEvent
//  ******************

class ProgChangeSeqEvent:
    public SeqEvent {
 public:
  ProgChangeSeqEvent(SeqTrack *pTrack,
                     uint32_t programNumber,
                     uint32_t offset = 0,
                     uint32_t length = 0,
                     const std::wstring &_name = L"");
  ~ProgChangeSeqEvent() override = default;
  EventType GetEventType() override { return EVENTTYPE_PROGCHANGE; }
  DESCRIPTION(L"Program Number: " << progNum)

 public:
  uint32_t progNum;
};

//  *************
//  TempoSeqEvent
//  *************

class TempoSeqEvent:
    public SeqEvent {
 public:
  TempoSeqEvent(SeqTrack *pTrack,
                double beatsperminute,
                uint32_t offset = 0,
                uint32_t length = 0,
                const std::wstring &_name = L"");
  ~TempoSeqEvent() override = default;
  EventType GetEventType() override { return EVENTTYPE_TEMPO; }
  Icon GetIcon() override { return ICON_TEMPO; }
  DESCRIPTION(L"BPM: " << bpm)

 public:
  double bpm;
};

//  ******************
//  TempoSlideSeqEvent
//  ******************

class TempoSlideSeqEvent:
    public SeqEvent {
 public:
  TempoSlideSeqEvent(SeqTrack *pTrack,
                     double targBPM,
                     uint32_t duration,
                     uint32_t offset = 0,
                     uint32_t length = 0,
                     const std::wstring &_name = L"");
  ~TempoSlideSeqEvent() override = default;
  Icon GetIcon() override { return ICON_TEMPO; }
  DESCRIPTION(L"BPM: " << targbpm << L"  Duration: " << dur)

 public:
  double targbpm;
  uint32_t dur;
};

//  ***************
//  TimeSigSeqEvent
//  ***************

class TimeSigSeqEvent:
    public SeqEvent {
 public:
  TimeSigSeqEvent(SeqTrack *pTrack,
                  uint8_t numerator,
                  uint8_t denominator,
                  uint8_t theTicksPerQuarter,
                  uint32_t offset = 0,
                  uint32_t length = 0,
                  const std::wstring &_name = L"");
  ~TimeSigSeqEvent() override = default;
  EventType GetEventType() override { return EVENTTYPE_TIMESIG; }
  DESCRIPTION(L"Signature: " << numer << L"/" << denom << L"  Ticks Per Quarter: " << ticksPerQuarter)

 public:
  uint8_t numer;
  uint8_t denom;
  uint8_t ticksPerQuarter;
};

//  **************
//  MarkerSeqEvent
//  **************

class MarkerSeqEvent:
    public SeqEvent {
 public:
  MarkerSeqEvent(SeqTrack *pTrack,
                 const std::string &markername,
                 uint8_t _databyte1,
                 uint8_t _databyte2,
                 uint32_t offset = 0,
                 uint32_t length = 0,
                 const std::wstring &_name = L"",
                 uint8_t _color = CLR_MARKER)
      : SeqEvent(pTrack, offset, length, _name, _color), databyte1(_databyte1), databyte2(_databyte2) { }
  EventType GetEventType() override { return EVENTTYPE_MARKER; }

 public:
  uint8_t databyte1;
  uint8_t databyte2;
};

//  ***************
//  VibratoSeqEvent
//  ***************

//class VibratoSeqEvent :
//	public SeqEvent
//{
//public:
//	VibratoSeqEvent(SeqTrack* pTrack, uint8_t detph, uint32_t offset = 0, uint32_t length = 0, const std::wstring& name = L"")
//	: SeqEvent(pTrack, offset, length, _name), depth(depth)
//	{}
//	EventType GetEventType() override { return EVENTTYPE_VIBRATO; }
//
//public:
//	uint8_t depth;
//};

//  ****************
//  TrackEndSeqEvent
//  ****************

class TrackEndSeqEvent:
    public SeqEvent {
 public:
  TrackEndSeqEvent(SeqTrack *pTrack, uint32_t offset = 0, uint32_t length = 0, const std::wstring &_name = L"")
      : SeqEvent(pTrack, offset, length, _name, CLR_TRACKEND) { }
  EventType GetEventType() override { return EVENTTYPE_TRACKEND; }
};

//  *******************
//  LoopForeverSeqEvent
//  *******************

class LoopForeverSeqEvent:
    public SeqEvent {
 public:
  LoopForeverSeqEvent(SeqTrack *pTrack, uint32_t offset = 0, uint32_t length = 0, const std::wstring &_name = L"")
      : SeqEvent(pTrack, offset, length, _name, CLR_LOOPFOREVER) { }
  EventType GetEventType() override { return EVENTTYPE_LOOPFOREVER; }
};