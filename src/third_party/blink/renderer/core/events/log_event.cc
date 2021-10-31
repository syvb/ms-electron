// Copyright (c) 2019 Microsoft Corporation.

#include "microsoft/src/third_party/blink/renderer/core/events/log_event.h"

#include "third_party/blink/renderer/core/event_interface_names.h"
#include "third_party/blink/renderer/core/event_type_names.h"

namespace blink {

LogEvent::LogEvent(LogLevel level, const String& message)
    : Event(event_type_names::kMsLogEvent, Bubbles::kNo, Cancelable::kNo),
      m_level(level),
      m_message(message) {}

LogEvent::~LogEvent() = default;

const AtomicString& LogEvent::InterfaceName() const {
  return event_interface_names::kLogEvent;
}

}  // namespace blink
