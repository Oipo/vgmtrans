#pragma once
#include "VGMItem.h"
#include "Loop.h"
#include "Menu.h"

class VGMSampColl;


enum WAVE_TYPE {
  WT_UNDEFINED,
  WT_PCM8,
  WT_PCM16
};


class VGMSamp:
    public VGMItem {
 public:
  BEGIN_MENU(VGMSamp)
      MENU_ITEM(VGMSamp, OnSaveAsWav, L"Convert to WAV file")
  END_MENU()


  VGMSamp(VGMSampColl *sampColl, uint32_t offset = 0, uint32_t length = 0,
          uint32_t dataOffset = 0, uint32_t dataLength = 0, uint8_t channels = 1, uint16_t bps = 16,
          uint32_t rate = 0, std::wstring name = L"Sample");
  ~VGMSamp() override;

  Icon GetIcon() override { return ICON_SAMP; };

  virtual double GetCompressionRatio(); // ratio of space conserved.  should generally be > 1
  // used to calculate both uncompressed sample size and loopOff after conversion
  virtual void ConvertToStdWave(uint8_t *buf);

  inline void SetWaveType(WAVE_TYPE type) { waveType = type; }
  inline void SetBPS(uint16_t theBPS) { bps = theBPS; }
  inline void SetRate(uint32_t theRate) { rate = theRate; }
  inline void SetNumChannels(uint8_t nChannels) { channels = nChannels; }
  inline void SetDataOffset(uint32_t theDataOff) { dataOff = theDataOff; }
  inline void SetDataLength(uint32_t theDataLength) { dataLength = theDataLength; }
  [[nodiscard]] inline int GetLoopStatus() const { return loop.loopStatus; }
  inline void SetLoopStatus(int loopStat) { loop.loopStatus = loopStat; }
  inline void SetLoopOffset(uint32_t loopStart) { loop.loopStart = loopStart; }
  [[nodiscard]] inline int GetLoopLength() const { return loop.loopLength; }
  inline void SetLoopLength(uint32_t theLoopLength) { loop.loopLength = theLoopLength; }
  inline void SetLoopStartMeasure(LoopMeasure measure) { loop.loopStartMeasure = measure; }
  inline void SetLoopLengthMeasure(LoopMeasure measure) { loop.loopLengthMeasure = measure; }

  bool OnSaveAsWav();
  bool SaveAsWav(const std::wstring &filepath);

 public:
  WAVE_TYPE waveType;
  uint32_t dataOff;    //offset of original sample data
  uint32_t dataLength;
  uint16_t bps;        //bits per sample
  uint32_t rate;        //sample rate in herz (samples per second)
  uint8_t channels;    //mono or stereo?
  uint32_t ulUncompressedSize;

  bool bPSXLoopInfoPrioritizing;
  Loop loop;

  uint8_t unityKey;
  short fineTune;
  double volume;    //as percent of full volume.  This will be converted to attenuation for SynthFile

  long pan;

  VGMSampColl *parSampColl;
  std::wstring sampName;
};