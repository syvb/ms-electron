// Copyright (c) 2019 Microsoft Corporation.

#ifndef SRC_THIRD_PARTY_BLINK_RENDERER_CORE_HTML_HTML_SKYPE_VIDEO_ELEMENT_H_
#define SRC_THIRD_PARTY_BLINK_RENDERER_CORE_HTML_HTML_SKYPE_VIDEO_ELEMENT_H_

#include <string>

#include "core/core_export.h"
#include "core/html/media/html_video_element.h"
#include "microsoft/src/third_party/blink/platform/skype_media_player_client.h"

namespace blink {

class CORE_EXPORT HTMLSkypeVideoElement final : public HTMLVideoElement,
                                                private SkypeMediaPlayerClient {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static constexpr auto kApiVersion = 2;

  explicit HTMLSkypeVideoElement(Document&);

  unsigned videoWidth() const;
  unsigned videoHeight() const;

  unsigned rendererWidth() const;
  unsigned rendererHeight() const;

  bool backgroundRendering() const;
  bool textureSharingSupported() const;

  String scalingMode() const;
  void setScalingMode(const String& value);

  bool bufferSharingEnabled() const;
  void setBufferSharingEnabled(bool value);

  void loadSync(const AtomicString& bufferName,
                ExceptionState& exception_state);  // NOLINT

 private:
  // Node override.
  Node::InsertionNotificationRequest InsertedInto(ContainerNode& node) override;
  void RemovedFrom(ContainerNode& node) override;

  // HTMLMediaElement
  void CheckVideoPosition() override;

  // WebMediaPlayerClient
  SkypeMediaPlayerClient* GetSkypeMediaPlayerClient() override;

  // SkypeMediaPlayerClient
  bool UseBufferSharing() const override;

  void BackgroundRenderingChanged() override;
  void TextureSharingSupportedChanged() override;
  void RendererSizeChanged() override;
  void VideoPositionChanged() override;

  void LogEvent(SkypeMediaPlayerClient::LogLevel level,
                const std::string& message) override;

  SkypeMediaPlayer* GetSkypeMediaPlayer() const;

  String GetObjectFit() const;
  String GetObjectPosition() const;

  void SetStyleProperty(const String& name, const String& value);

  enum class ScalingMode { Stretch, Crop, Fit };
  ScalingMode scaling_mode_ = ScalingMode::Fit;

  bool buffer_sharing_enabled_ = false;
};

inline HTMLVideoElement* ToHTMLVideoElementEx(Node* node) {
  return IsA<HTMLSkypeVideoElement>(node) ? To<HTMLSkypeVideoElement>(node)
                                          : To<HTMLVideoElement>(node);
}

}  // namespace blink

#endif  // SRC_THIRD_PARTY_BLINK_RENDERER_CORE_HTML_HTML_SKYPE_VIDEO_ELEMENT_H_
