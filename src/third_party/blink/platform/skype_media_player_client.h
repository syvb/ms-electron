// Copyright (c) 2019 Microsoft Corporation.

#ifndef SRC_THIRD_PARTY_BLINK_PLATFORM_SKYPE_MEDIA_PLAYER_CLIENT_H_
#define SRC_THIRD_PARTY_BLINK_PLATFORM_SKYPE_MEDIA_PLAYER_CLIENT_H_

#include <string>

namespace blink {

class SkypeMediaPlayerClient {
 public:
  virtual bool UseBufferSharing() const = 0;

  enum LogLevel {
    kLogLevelDefault,
    kLogLevelDebug,
    kLogLevelInfo,
    kLogLevelWarning,
    kLogLevelError
  };

  virtual void BackgroundRenderingChanged() = 0;
  virtual void TextureSharingSupportedChanged() = 0;
  virtual void RendererSizeChanged() = 0;
  virtual void VideoPositionChanged() = 0;

  virtual void LogEvent(LogLevel level, const std::string& message) = 0;
};

}  // namespace blink

#endif  // SRC_THIRD_PARTY_BLINK_PLATFORM_SKYPE_MEDIA_PLAYER_CLIENT_H_
