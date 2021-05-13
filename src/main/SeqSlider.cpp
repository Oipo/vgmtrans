#include "pch.h"
#include "SeqSlider.h"

template<typename TNumber>
SeqSlider<TNumber>::SeqSlider(SeqTrack *_track,
                              uint32_t _time,
                              uint32_t _duration,
                              TNumber _initialValue,
                              TNumber _targetValue) :
    track(_track),
    time(_time),
    duration(_duration),
    initialValue(_initialValue),
    targetValue(_targetValue) {
}

template<typename TNumber>
SeqSlider<TNumber>::~SeqSlider() = default;

template<typename TNumber>
TNumber SeqSlider<TNumber>::get(uint32_t _time) const {
  if (!isActive(_time))
    return 0;

  // linear interpolation
  uint32_t step = _time - this->time;
  double alpha = static_cast<double>(step) / static_cast<double>(this->duration);
  return static_cast<TNumber> (initialValue * (1.0 - alpha) + targetValue * alpha);
}

template<typename TNumber>
void SeqSlider<TNumber>::write(uint32_t _time) const {
  if (changesAt(_time))
    writeMessage(get(_time));
}

template<typename TNumber>
bool SeqSlider<TNumber>::changesAt(uint32_t _time) const {
  if (!isActive(_time))
    return false;

  if (_time == this->time)
    return true;

  return get(_time) != get(_time - 1);
}

template<typename TNumber>
bool SeqSlider<TNumber>::isStarted(uint32_t _time) const {
  return _time >= this->time;
}

template<typename TNumber>
bool SeqSlider<TNumber>::isActive(uint32_t _time) const {
  return isStarted(_time) && _time <= this->time + this->duration;
}

VolSlider::VolSlider(SeqTrack *_track, uint32_t _time, uint32_t _duration, uint8_t _initialValue, uint8_t _targetValue) :
    SeqSlider(_track, _time, _duration, _initialValue, _targetValue) {
}

void VolSlider::writeMessage(uint8_t value) const {
  track->AddVolNoItem(value);
}

MasterVolSlider::MasterVolSlider(SeqTrack *_track,
                                 uint32_t _time,
                                 uint32_t _duration,
                                 uint8_t _initialValue,
                                 uint8_t _targetValue) :
    SeqSlider(_track, _time, _duration, _initialValue, _targetValue) {
}

void MasterVolSlider::writeMessage(uint8_t value) const {
  track->AddMasterVolNoItem(value);
}

ExpressionSlider::ExpressionSlider(SeqTrack *_track,
                                   uint32_t _time,
                                   uint32_t _duration,
                                   uint8_t _initialValue,
                                   uint8_t _targetValue) :
    SeqSlider(_track, _time, _duration, _initialValue, _targetValue) {
}

void ExpressionSlider::writeMessage(uint8_t value) const {
  track->AddExpressionNoItem(value);
}

PanSlider::PanSlider(SeqTrack *_track, uint32_t _time, uint32_t _duration, uint8_t _initialValue, uint8_t _targetValue) :
    SeqSlider(_track, _time, _duration, _initialValue, _targetValue) {
}

void PanSlider::writeMessage(uint8_t value) const {
  track->AddPanNoItem(value);
}
