// Copyright (c) 2019 Microsoft Corporation.

#ifndef SRC_THIRD_PARTY_BLINK_RENDERER_CORE_EVENTS_LOG_EVENT_H_
#define SRC_THIRD_PARTY_BLINK_RENDERER_CORE_EVENTS_LOG_EVENT_H_

#include "core/dom/events/event.h"

namespace blink {

class LogEvent final : public Event {
  DEFINE_WRAPPERTYPEINFO();

 public:
  enum LogLevel {
    kLogLevelDefault,
    kLogLevelDebug,
    kLogLevelInfo,
    kLogLevelWarning,
    kLogLevelError,
  };

  static LogEvent* Create(LogLevel level, const String& message) {
    return MakeGarbageCollected<LogEvent>(level, message);
  }

  LogEvent(LogLevel level, const String& message);
  ~LogEvent() override;

  const AtomicString& InterfaceName() const override;

  LogLevel level() const { return m_level; }
  const String& message() const { return m_message; }

 private:
  LogLevel m_level = kLogLevelDefault;
  String m_message;
};

}  // namespace blink

#endif  // SRC_THIRD_PARTY_BLINK_RENDERER_CORE_EVENTS_LOG_EVENT_H_
