#pragma once

#include <string>

enum LogLevel { LOG_LEVEL_ERR, LOG_LEVEL_WARN, LOG_LEVEL_INFO, LOG_LEVEL_DEBUG };

class LogItem {
 public:
  LogItem();
  LogItem(const wchar_t *text, LogLevel level, const wchar_t *source);
  LogItem(const std::wstring &text, LogLevel level, const std::wstring &source);
  virtual ~LogItem();

  [[nodiscard]] std::wstring GetText() const;
  [[nodiscard]] const wchar_t *GetCText() const;
  [[nodiscard]] uint64_t GetTime() const;
  [[nodiscard]] LogLevel GetLogLevel() const;
  [[nodiscard]] std::wstring GetSource() const;
  [[nodiscard]] const wchar_t *GetCSource() const;

 protected:
  std::wstring text;
  std::wstring source;
  uint64_t time;
  LogLevel level;
};
