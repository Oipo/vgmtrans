#include "pch.h"

#include "LogItem.h"
#include <chrono>

LogItem::LogItem() :
    text(),
    source(),
    time(std::chrono::system_clock::now().time_since_epoch().count()),
    level(LOG_LEVEL_ERR) {
}

LogItem::LogItem(const wchar_t *_text, LogLevel _level, const wchar_t *_source) :
    text(_text ? _text : L""),
    source(_source ? _source : L""),
    time(std::chrono::system_clock::now().time_since_epoch().count()),
    level(_level) {
}

LogItem::LogItem(const std::wstring &_text, LogLevel _level, const std::wstring &_source) :
    text(_text),
    source(_source),
    time(std::chrono::system_clock::now().time_since_epoch().count()),
    level(_level) {
}

LogItem::~LogItem() = default;

std::wstring LogItem::GetText() const {
  return std::wstring(text);
}

const wchar_t *LogItem::GetCText() const {
  return text.c_str();
}

uint64_t LogItem::GetTime() const {
  return time;
}

LogLevel LogItem::GetLogLevel() const {
  return level;
}

std::wstring LogItem::GetSource() const {
  return std::wstring(source);
}

const wchar_t *LogItem::GetCSource() const {
  return source.c_str();
}
