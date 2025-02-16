#pragma once
#include <iterator>
#include <string>
#include <vector>
#include <list>

class VGMSeq;

class MidiFile;
class MidiTrack;
class MidiEvent;
class DurNoteEvent;
class NoteEvent;

#define PRIORITY_LOWEST 127
#define PRIORITY_LOWER 96
#define PRIORITY_LOW 64
#define PRIORITY_MIDDLE 0
#define PRIORITY_HIGH -64
#define PRIORITY_HIGHER -96
#define PRIORITY_HIGHEST -128

typedef enum {
  MIDIEVENT_UNDEFINED,
  MIDIEVENT_MASTERVOL,
  MIDIEVENT_GLOBALTRANSPOSE,
  MIDIEVENT_BANKSELECT,
  MIDIEVENT_BANKSELECTFINE,
  MIDIEVENT_MARKER,
  MIDIEVENT_NOTEON,
  MIDIEVENT_NOTEOFF,
  MIDIEVENT_DURNOTE,
  MIDIEVENT_REST,
  MIDIEVENT_EXPRESSION,
  MIDIEVENT_EXPRESSIONSLIDE,
  MIDIEVENT_VOLUME,
  MIDIEVENT_VOLUMESLIDE,
  MIDIEVENT_PAN,
  MIDIEVENT_PROGRAMCHANGE,
  MIDIEVENT_PITCHBEND,
  MIDIEVENT_TEMPO,
  MIDIEVENT_TIMESIG,
  MIDIEVENT_MODULATION,
  MIDIEVENT_BREATH,
  MIDIEVENT_SUSTAIN,
  MIDIEVENT_PORTAMENTO,
  MIDIEVENT_PORTAMENTOTIME,
  MIDIEVENT_MONO,
  MIDIEVENT_LFO,
  MIDIEVENT_VIBRATO,
  MIDIEVENT_ENDOFTRACK,
  MIDIEVENT_TEXT,
  MIDIEVENT_RESET
} MidiEventType;

class MidiTrack {
 public:
  MidiTrack(MidiFile *parentSeq, bool bMonophonic);
  virtual ~MidiTrack();

  void Sort();
  void WriteTrack(std::vector<uint8_t> &buf) const;

  //void SetChannel(int theChannel);
  void SetChannelGroup(int theChannelGroup);

  uint32_t GetDelta() const;
  void SetDelta(uint32_t NewDelta);
  void AddDelta(uint32_t AddDelta);
  void SubtractDelta(uint32_t SubtractDelta);
  void ResetDelta();

  void AddNoteOn(uint8_t channel, int8_t key, int8_t vel);
  void InsertNoteOn(uint8_t channel, int8_t key, int8_t vel, uint32_t absTime);
  void AddNoteOff(uint8_t channel, int8_t key);
  void InsertNoteOff(uint8_t channel, int8_t key, uint32_t absTime);
  void AddNoteByDur(uint8_t channel, int8_t key, int8_t vel, uint32_t duration);
  void AddNoteByDur_TriAce(uint8_t channel, int8_t key, int8_t vel, uint32_t duration);
  void InsertNoteByDur(uint8_t channel, int8_t key, int8_t vel, uint32_t duration, uint32_t absTime);
  void PurgePrevNoteOffs();
  void PurgePrevNoteOffs(uint32_t absTime);
  void AddControllerEvent(uint8_t channel,
                          uint8_t controllerNum,
                          uint8_t theDataByte); // This function should be used for only redirection output of MIDI-like formats
  void InsertControllerEvent(uint8_t channel,
                             uint8_t controllerNum,
                             uint8_t theDataByte,
                             uint32_t absTime); // This function should be used for only redirection output of MIDI-like formats
  //void AddVolMarker(uint8_t channel, uint8_t vol, int8_t priority = PRIORITY_HIGHER);
  //void InsertVolMarker(uint8_t channel, uint8_t vol, uint32_t absTime, int8_t priority = PRIORITY_HIGHER);
  void AddVol(uint8_t channel, uint8_t vol/*, int8_t priority = PRIORITY_MIDDLE*/);
  void InsertVol(uint8_t channel, uint8_t vol, uint32_t absTime/*, int8_t priority = PRIORITY_MIDDLE*/);
  void AddMasterVol(uint8_t channel, uint8_t mastVol/*, int8_t priority = PRIORITY_HIGHER*/);
  void InsertMasterVol(uint8_t channel, uint8_t mastVol, uint32_t absTime/*, int8_t priority = PRIORITY_HIGHER*/);
  void AddPan(uint8_t channel, uint8_t pan);
  void InsertPan(uint8_t channel, uint8_t pan, uint32_t absTime);
  void AddExpression(uint8_t channel, uint8_t expression);
  void InsertExpression(uint8_t channel, uint8_t expression, uint32_t absTime);
  void AddReverb(uint8_t channel, uint8_t reverb);
  void InsertReverb(uint8_t channel, uint8_t reverb, uint32_t absTime);
  void AddModulation(uint8_t channel, uint8_t depth);
  void InsertModulation(uint8_t channel, uint8_t depth, uint32_t absTime);
  void AddBreath(uint8_t channel, uint8_t depth);
  void InsertBreath(uint8_t channel, uint8_t depth, uint32_t absTime);
  void AddSustain(uint8_t channel, uint8_t depth);
  void InsertSustain(uint8_t channel, uint8_t depth, uint32_t absTime);
  void AddPortamento(uint8_t channel, bool bOn);
  void InsertPortamento(uint8_t channel, bool bOn, uint32_t absTime);
  void AddPortamentoTime(uint8_t channel, uint8_t time);
  void InsertPortamentoTime(uint8_t channel, uint8_t time, uint32_t absTime);
  void AddMono(uint8_t channel);
  void InsertMono(uint8_t channel, uint32_t absTime);

  void AddPitchBend(uint8_t channel, int16_t bend);
  void InsertPitchBend(uint8_t channel, short bend, uint32_t absTime);
  void AddPitchBendRange(uint8_t channel, uint8_t semitones, uint8_t cents);
  void InsertPitchBendRange(uint8_t channel, uint8_t semitones, uint8_t cents, uint32_t absTime);
  void AddFineTuning(uint8_t channel, uint8_t msb, uint8_t lsb);
  void InsertFineTuning(uint8_t channel, uint8_t msb, uint8_t lsb, uint32_t absTime);
  void AddFineTuning(uint8_t channel, double cents);
  void InsertFineTuning(uint8_t channel, double cents, uint32_t absTime);
  void AddCoarseTuning(uint8_t channel, uint8_t msb, uint8_t lsb);
  void InsertCoarseTuning(uint8_t channel, uint8_t msb, uint8_t lsb, uint32_t absTime);
  void AddCoarseTuning(uint8_t channel, double semitones);
  void InsertCoarseTuning(uint8_t channel, double semitones, uint32_t absTime);
  void AddModulationDepthRange(uint8_t channel, uint8_t msb, uint8_t lsb);
  void InsertModulationDepthRange(uint8_t channel, uint8_t msb, uint8_t lsb, uint32_t absTime);
  void AddModulationDepthRange(uint8_t channel, double semitones);
  void InsertModulationDepthRange(uint8_t channel, double semitones, uint32_t absTime);
  //void AddTranspose(uint8_t channel, int transpose);
  void AddProgramChange(uint8_t channel, uint8_t progNum);
  void AddBankSelect(uint8_t channel, uint8_t bank);
  void AddBankSelectFine(uint8_t channel, uint8_t lsb);
  void InsertBankSelect(uint8_t channel, uint8_t bank, uint32_t absTime);

  void AddTempo(uint32_t microSeconds);
  void AddTempoBPM(double BPM);
  void InsertTempo(uint32_t microSeconds, uint32_t absTime);
  void InsertTempoBPM(double BPM, uint32_t absTime);
  void AddTimeSig(uint8_t numer, uint8_t denom, uint8_t clicksPerQuarter);
  void InsertTimeSig(uint8_t numer, uint8_t denom, uint8_t ticksPerQuarter, uint32_t absTime);
  void AddEndOfTrack();
  void InsertEndOfTrack(uint32_t absTime);
  void AddText(const std::wstring &wstr);
  void InsertText(const std::wstring &wstr, uint32_t absTime);
  void AddSeqName(const std::wstring &wstr);
  void InsertSeqName(const std::wstring &wstr, uint32_t absTime);
  void AddTrackName(const std::wstring &wstr);
  void InsertTrackName(const std::wstring &wstr, uint32_t absTime);
  void AddGMReset();
  void InsertGMReset(uint32_t absTime);
  void AddGM2Reset();
  void InsertGM2Reset(uint32_t absTime);
  void AddGSReset();
  void InsertGSReset(uint32_t absTime);
  void AddXGReset();
  void InsertXGReset(uint32_t absTime);

  // SPECIAL EVENTS
  //void AddTranspose(int8_t semitones);
  void InsertGlobalTranspose(uint32_t absTime, int8_t semitones);
  void AddMarker(uint8_t channel,
                 const std::string &markername,
                 uint8_t databyte1,
                 uint8_t databyte2,
                 int8_t priority = PRIORITY_MIDDLE);

 public:
  MidiFile *parentSeq;
  bool bMonophonic;

  bool bHasEndOfTrack;
  int channelGroup;

  uint32_t DeltaTime;            //a time value to be used for AddEvent

  DurNoteEvent *prevDurEvent;
  std::vector<NoteEvent *> prevDurNoteOffs;
  int8_t prevKey;
  //uint8_t mastVol;
  //uint8_t vol;
  //uint8_t expression;
  bool bSustain;

  std::vector<MidiEvent *> aEvents;
};

class MidiFile {
 public:
  MidiFile(VGMSeq *assocSeq);
//  MidiFile(uint32_t thePpqn);
  ~MidiFile();
  MidiTrack *AddTrack();
  MidiTrack *InsertTrack(uint32_t trackNum);
  int GetMidiTrackIndex(MidiTrack *midiTrack);
  void SetPPQN(uint16_t ppqn);
  uint32_t GetPPQN() const;
  void WriteMidiToBuffer(std::vector<uint8_t> &buf);
  void Sort();
  bool SaveMidiFile(const std::wstring &filepath);

 protected:
  //bool bAddedTempo;
  //bool bAddedTimeSig;

 public:
  VGMSeq *assocSeq;
  uint16_t ppqn;

  std::vector<MidiTrack *> aTracks;
  MidiTrack globalTrack;            //events in the globalTrack will be copied into every other track
  int8_t globalTranspose;
  bool bMonophonicTracks;
};

class MidiEvent {
 public:
  MidiEvent(MidiTrack *thePrntTrk, uint32_t absoluteTime, uint8_t theChannel, int8_t thePriority);
  virtual ~MidiEvent();
  virtual MidiEventType GetEventType() = 0;
  void WriteVarLength(std::vector<uint8_t> &buf, uint32_t value);
  //virtual void PrepareWrite(void/*vector<MidiEvent*> & aEvents*/);
  virtual uint32_t WriteEvent(std::vector<uint8_t> &buf, uint32_t time) = 0;
  uint32_t WriteSysexEvent(std::vector<uint8_t> &buf, uint32_t time, uint8_t *data, size_t dataSize);
  uint32_t WriteMetaEvent(std::vector<uint8_t> &buf, uint32_t time, uint8_t metaType, uint8_t *data, size_t dataSize);
  uint32_t WriteMetaTextEvent(std::vector<uint8_t> &buf, uint32_t time, uint8_t metaType, std::wstring wstr);

  static std::wstring GetNoteName(int noteNumber);

  bool operator<(const MidiEvent &) const;
  bool operator>(const MidiEvent &) const;

  MidiTrack *prntTrk;
  int8_t priority;
  uint8_t channel;
  uint32_t AbsTime;            //absolute time... the number of ticks from the very beginning of the sequence at which this event occurs
};

class PriorityCmp {
 public:
  bool operator()(const MidiEvent *a, const MidiEvent *b) const {
    return (a->priority < b->priority);
  }
};

class AbsTimeCmp {
 public:
  bool operator()(const MidiEvent *a, const MidiEvent *b) const {
    return (a->AbsTime < b->AbsTime);
  }
};

class NoteEvent
    : public MidiEvent {
 public:
  NoteEvent
      (MidiTrack *_prntTrk, uint8_t _channel, uint32_t _absoluteTime, bool bnoteDown, uint8_t theKey, uint8_t theVel = 64);
  MidiEventType GetEventType() override { return MIDIEVENT_NOTEON; }
  uint32_t WriteEvent(std::vector<uint8_t> &buf, uint32_t time) override;

  bool bNoteDown;
  bool bUsePrevKey;
  int8_t key;
  int8_t vel;
};

//class DurNoteEvent
//	: public MidiEvent
//{
//public:
//	DurNoteEvent(MidiTrack* prntTrk, uint8_t channel, uint32_t absoluteTime, uint8_t theKey, uint8_t theVel, uint32_t theDur);
//	//virtual void PrepareWrite(std::vector<MidiEvent*> & aEvents);
//	virtual uint32_t WriteEvent(std::vector<uint8_t> & buf, uint32_t time);
//
//	bool bNoteDown;
//	int8_t key;
//	int8_t vel;
//	uint32_t duration;
//};

class ControllerEvent
    : public MidiEvent {
 public:
  ControllerEvent(MidiTrack *prntTrk,
                  uint8_t channel,
                  uint32_t absoluteTime,
                  uint8_t controllerNum,
                  uint8_t theDataByte,
                  int8_t thePriority = PRIORITY_MIDDLE);
  MidiEventType GetEventType() override { return MIDIEVENT_UNDEFINED; }
  uint32_t WriteEvent(std::vector<uint8_t> &buf, uint32_t time) override;

  uint8_t controlNum;
  uint8_t dataByte;
};

class VolumeEvent
    : public ControllerEvent {
 public:
  VolumeEvent(MidiTrack *_prntTrk, uint8_t _channel, uint32_t _absoluteTime, uint8_t volume)
      : ControllerEvent(_prntTrk, _channel, _absoluteTime, 7, volume, PRIORITY_MIDDLE) { }
  MidiEventType GetEventType() override { return MIDIEVENT_VOLUME; }
};

class ExpressionEvent
    : public ControllerEvent {
 public:
  ExpressionEvent(MidiTrack *_prntTrk, uint8_t _channel, uint32_t _absoluteTime, uint8_t expression)
      : ControllerEvent(_prntTrk, _channel, _absoluteTime, 11, expression, PRIORITY_MIDDLE) { }
  MidiEventType GetEventType() override { return MIDIEVENT_EXPRESSION; }
};

class SustainEvent
    : public ControllerEvent {
 public:
  SustainEvent(MidiTrack *_prntTrk, uint8_t _channel, uint32_t _absoluteTime, uint8_t depth)
      : ControllerEvent(_prntTrk, _channel, _absoluteTime, 64, depth, PRIORITY_MIDDLE) { }
  MidiEventType GetEventType() override { return MIDIEVENT_SUSTAIN; }
};

class PortamentoEvent
    : public ControllerEvent {
 public:
  PortamentoEvent(MidiTrack *_prntTrk, uint8_t _channel, uint32_t _absoluteTime, uint8_t bOn)
      : ControllerEvent(_prntTrk, _channel, _absoluteTime, 65, (bOn) ? 0x7F : 0, PRIORITY_MIDDLE) { }
  MidiEventType GetEventType() override { return MIDIEVENT_PORTAMENTO; }
};

class PortamentoTimeEvent
    : public ControllerEvent {
 public:
  PortamentoTimeEvent(MidiTrack *_prntTrk, uint8_t _channel, uint32_t _absoluteTime, uint8_t time)
      : ControllerEvent(_prntTrk, _channel, _absoluteTime, 5, time, PRIORITY_MIDDLE) { }
  MidiEventType GetEventType() override { return MIDIEVENT_PORTAMENTOTIME; }
};

class PanEvent
    : public ControllerEvent {
 public:
  PanEvent(MidiTrack *_prntTrk, uint8_t _channel, uint32_t _absoluteTime, uint8_t pan)
      : ControllerEvent(_prntTrk, _channel, _absoluteTime, 10, pan, PRIORITY_MIDDLE) { }
  MidiEventType GetEventType() override { return MIDIEVENT_PAN; }
};

class ModulationEvent
    : public ControllerEvent {
 public:
  ModulationEvent(MidiTrack *_prntTrk, uint8_t _channel, uint32_t _absoluteTime, uint8_t depth)
      : ControllerEvent(_prntTrk, _channel, _absoluteTime, 1, depth, PRIORITY_MIDDLE) { }
  MidiEventType GetEventType() override { return MIDIEVENT_MODULATION; }
};

class BreathEvent
    : public ControllerEvent {
 public:
  BreathEvent(MidiTrack *_prntTrk, uint8_t _channel, uint32_t _absoluteTime, uint8_t depth)
      : ControllerEvent(_prntTrk, _channel, _absoluteTime, 2, depth, PRIORITY_MIDDLE) { }
  MidiEventType GetEventType() override { return MIDIEVENT_BREATH; }
};

class BankSelectEvent
    : public ControllerEvent {
 public:
  BankSelectEvent(MidiTrack *_prntTrk, uint8_t _channel, uint32_t _absoluteTime, uint8_t bank)
      : ControllerEvent(_prntTrk, _channel, _absoluteTime, 0, bank, PRIORITY_HIGH) { }
  MidiEventType GetEventType() override { return MIDIEVENT_BANKSELECT; }
};

class BankSelectFineEvent
    : public ControllerEvent {
 public:
  BankSelectFineEvent(MidiTrack *_prntTrk, uint8_t _channel, uint32_t _absoluteTime, uint8_t bank)
      : ControllerEvent(_prntTrk, _channel, _absoluteTime, 32, bank, PRIORITY_HIGH) { }
  MidiEventType GetEventType() override { return MIDIEVENT_BANKSELECTFINE; }
};

/*
class VolEvent
	: public ControllerEvent
{
public:
	VolEvent(MidiTrack* prntTrk, uint8_t channel, uint32_t absoluteTime, uint8_t theVol, int8_t thePriority = PRIORITY_MIDDLE);
	virtual VolEvent* MakeCopy();
	//virtual uint32_t WriteEvent(std::vector<uint8_t> & buf, uint32_t time);

	uint8_t vol;
};

class VolMarkerEvent
	: public MidiEvent
{
public:
	VolMarkerEvent(MidiTrack* prntTrk, uint8_t channel, uint32_t absoluteTime, uint8_t theVol, int8_t thePriority = PRIORITY_HIGHER);
	virtual VolMarkerEvent* MakeCopy();
	virtual uint32_t WriteEvent(std::vector<uint8_t> & buf, uint32_t time);

	uint8_t vol;
};*/

class MastVolEvent
    : public MidiEvent {
 public:
  MastVolEvent(MidiTrack *_prntTrk, uint8_t _channel, uint32_t _absoluteTime, uint8_t mastVol);
  MidiEventType GetEventType() override { return MIDIEVENT_MASTERVOL; }
  //virtual MastVolEvent* MakeCopy();
  uint32_t WriteEvent(std::vector<uint8_t> &buf, uint32_t time) override;

  uint8_t mastVol;
};
/*
class ExpressionEvent
	: public MidiEvent
{
public:
	ExpressionEvent(MidiTrack* prntTrk, uint8_t channel, uint32_t absoluteTime, uint8_t expression, int8_t thePriority = PRIORITY_HIGHER);
	virtual ExpressionEvent* MakeCopy();
	virtual uint32_t WriteEvent(std::vector<uint8_t> & buf, uint32_t time);

	uint8_t expression;
};*/

class MonoEvent
    : public ControllerEvent {
 public:
  MonoEvent(MidiTrack *_prntTrk, uint8_t _channel, uint32_t _absoluteTime)
      : ControllerEvent(_prntTrk, _channel, _absoluteTime, 126, 127, PRIORITY_MIDDLE) { }
  MidiEventType GetEventType() override { return MIDIEVENT_MONO; }
};

class ProgChangeEvent
    : public MidiEvent {
 public:
  ProgChangeEvent(MidiTrack *_prntTrk, uint8_t _channel, uint32_t _absoluteTime, uint8_t progNum);
  MidiEventType GetEventType() override { return MIDIEVENT_PROGRAMCHANGE; }
  //virtual ProgChangeEvent* MakeCopy();
  uint32_t WriteEvent(std::vector<uint8_t> &buf, uint32_t time) override;

  uint8_t programNum;
};

class PitchBendEvent
    : public MidiEvent {
 public:
  PitchBendEvent(MidiTrack *_prntTrk, uint8_t _channel, uint32_t _absoluteTime, int16_t bend);
  MidiEventType GetEventType() override { return MIDIEVENT_PITCHBEND; }
  //virtual PitchBendEvent* MakeCopy();
  uint32_t WriteEvent(std::vector<uint8_t> &buf, uint32_t time) override;

  int16_t bend;
};


class TempoEvent
    : public MidiEvent {
 public:
  TempoEvent(MidiTrack *prntTrk, uint32_t absoluteTime, uint32_t microSeconds);
  MidiEventType GetEventType() override { return MIDIEVENT_TEMPO; }
  //virtual TempoEvent* MakeCopy();
  uint32_t WriteEvent(std::vector<uint8_t> &buf, uint32_t time) override;

  uint32_t microSecs;
};

class TimeSigEvent
    : public MidiEvent {
 public:
  TimeSigEvent
      (MidiTrack *prntTrk, uint32_t absoluteTime, uint8_t numerator, uint8_t denominator, uint8_t clicksPerQuarter);
  MidiEventType GetEventType() override { return MIDIEVENT_TIMESIG; }
  //virtual TimeSigEvent* MakeCopy();
  uint32_t WriteEvent(std::vector<uint8_t> &buf, uint32_t time) override;

  uint8_t numer;
  uint8_t denom;
  uint8_t ticksPerQuarter;
};

class EndOfTrackEvent
    : public MidiEvent {
 public:
  EndOfTrackEvent(MidiTrack *prntTrk, uint32_t absoluteTime);
  MidiEventType GetEventType() override { return MIDIEVENT_ENDOFTRACK; }
  //virtual EndOfTrackEvent* MakeCopy();
  uint32_t WriteEvent(std::vector<uint8_t> &buf, uint32_t time) override;
};

class TextEvent
    : public MidiEvent {
 public:
  TextEvent(MidiTrack *prntTrk, uint32_t absoluteTime, const std::wstring &wstr);
  MidiEventType GetEventType() override { return MIDIEVENT_TEXT; }
  uint32_t WriteEvent(std::vector<uint8_t> &buf, uint32_t time) override;

  std::wstring text;
};

class SeqNameEvent
    : public MidiEvent {
 public:
  SeqNameEvent(MidiTrack *prntTrk, uint32_t absoluteTime, const std::wstring &wstr);
  MidiEventType GetEventType() override { return MIDIEVENT_TEXT; }
  uint32_t WriteEvent(std::vector<uint8_t> &buf, uint32_t time) override;

  std::wstring text;
};

class TrackNameEvent
    : public MidiEvent {
 public:
  TrackNameEvent(MidiTrack *prntTrk, uint32_t absoluteTime, const std::wstring &wstr);
  MidiEventType GetEventType() override { return MIDIEVENT_TEXT; }
  uint32_t WriteEvent(std::vector<uint8_t> &buf, uint32_t time) override;

  std::wstring text;
};

// SPECIAL EVENTS THAT AFFECT OTHER MIDI EVENTS RATHER THAN DIRECTLY OUTPUT TO THE FILE
class GlobalTransposeEvent
    : public MidiEvent {
 public:
  GlobalTransposeEvent(MidiTrack *prntTrk, uint32_t absoluteTime, int8_t semitones);
  MidiEventType GetEventType() override { return MIDIEVENT_GLOBALTRANSPOSE; }
  uint32_t WriteEvent(std::vector<uint8_t> &buf, uint32_t time) override;

  int8_t semitones;
};

class MarkerEvent
    : public MidiEvent {
 public:
  MarkerEvent(MidiTrack *_prntTrk,
              uint8_t _channel,
              uint32_t _absoluteTime,
              const std::string &_name,
              uint8_t _databyte1,
              uint8_t _databyte2,
              int8_t _thePriority = PRIORITY_MIDDLE)
      : MidiEvent(_prntTrk, _absoluteTime, _channel, _thePriority), name(_name), databyte1(_databyte1),
        databyte2(_databyte2) { }
  MidiEventType GetEventType() override { return MIDIEVENT_MARKER; }
  uint32_t WriteEvent(std::vector<uint8_t> &buf, uint32_t time) override { return time; }

  std::string name;
  uint8_t databyte1, databyte2;
};

class GMResetEvent
    : public MidiEvent {
 public:
  GMResetEvent(MidiTrack *prntTrk, uint32_t absoluteTime);
  MidiEventType GetEventType() override { return MIDIEVENT_RESET; }
  uint32_t WriteEvent(std::vector<uint8_t> &buf, uint32_t time) override;
};

class GM2ResetEvent
    : public MidiEvent {
 public:
  GM2ResetEvent(MidiTrack *prntTrk, uint32_t absoluteTime);
  MidiEventType GetEventType() override { return MIDIEVENT_RESET; }
  uint32_t WriteEvent(std::vector<uint8_t> &buf, uint32_t time) override;
};

class GSResetEvent
    : public MidiEvent {
 public:
  GSResetEvent(MidiTrack *prntTrk, uint32_t absoluteTime);
  MidiEventType GetEventType() override { return MIDIEVENT_RESET; }
  uint32_t WriteEvent(std::vector<uint8_t> &buf, uint32_t time) override;
};

class XGResetEvent
    : public MidiEvent {
 public:
  XGResetEvent(MidiTrack *prntTrk, uint32_t absoluteTime);
  MidiEventType GetEventType() override { return MIDIEVENT_RESET; }
  uint32_t WriteEvent(std::vector<uint8_t> &buf, uint32_t time) override;
};



/*
class AFX_EXT_CLASS ProgChangeEvent
	: public MidiEvent
{
	ProgChangeEvent();
	~ProgramChangeEvent();

	uint8_t progNum;
}

class AFX_EXT_CLASS ControllerEvent
	: public MidiEvent
{
public:
	NoteEvent();
	~NoteEvent();

	int8_t key;
	int8_t vel;
};*/

/*class AFX_EXT_CLASS NoteEventDur
	: public MidiEvent
{
public:
	NoteEvent();
	~NoteEvent();

	int8_t key;
	int8_t vel;
}*/