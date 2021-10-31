// Copyright (c) 2019 Microsoft Corporation.

#include "microsoft/src/third_party/blink/renderer/core/html/html_skype_video_element.h"

#include "base/strings/stringprintf.h"
#include "core/css/css_style_declaration.h"
#include "core/html_names.h"
#include "microsoft/src/third_party/blink/platform/skype_media_player.h"
#include "microsoft/src/third_party/blink/renderer/core/events/log_event.h"
#include "third_party/blink/public/platform/web_size.h"
#include "third_party/blink/renderer/core/dom/document.h"

namespace blink {

HTMLSkypeVideoElement::HTMLSkypeVideoElement(Document& document)
    : HTMLVideoElement(html_names::kSkypevideoTag, document) {}

unsigned HTMLSkypeVideoElement::videoWidth() const {
  auto* player = GetSkypeMediaPlayer();
  return player ? player->VideoSize().width() : 0;
}

unsigned HTMLSkypeVideoElement::videoHeight() const {
  auto* player = GetSkypeMediaPlayer();
  return player ? player->VideoSize().height() : 0;
}

unsigned HTMLSkypeVideoElement::rendererWidth() const {
  auto* player = GetSkypeMediaPlayer();
  return player ? player->RendererSize().width() : 0;
}

unsigned HTMLSkypeVideoElement::rendererHeight() const {
  auto* player = GetSkypeMediaPlayer();
  return player ? player->RendererSize().height() : 0;
}

bool HTMLSkypeVideoElement::backgroundRendering() const {
  auto* player = GetSkypeMediaPlayer();
  return player && player->IsBackgroundRendering();
}

bool HTMLSkypeVideoElement::textureSharingSupported() const {
  auto* player = GetSkypeMediaPlayer();
  return player && player->IsTextureSharingSupported();
}

void HTMLSkypeVideoElement::loadSync(const AtomicString& bufferName,
                                     ExceptionState& exception_state) {
  LogEvent(kLogLevelDebug,
           base::StringPrintf("HTMLSkypeVideoElement::loadSync('%s')",
                              bufferName.Utf8().data()));

  SetSrc("skypevideo:" + bufferName);

  ClearMediaPlayer();
  SelectMediaResource();

  if (!GetSkypeMediaPlayer()) {
    exception_state.ThrowDOMException(DOMExceptionCode::kUnknownError,
                                      "WebMediaPlayer failed");
    return;
  }

  if (!GetSkypeMediaPlayer()->IsInitialized()) {
    exception_state.ThrowDOMException(DOMExceptionCode::kUnknownError,
                                      "WebMediaPlayer not initialized");
    return;
  }
}

String HTMLSkypeVideoElement::scalingMode() const {
  switch (scaling_mode_) {
    case ScalingMode::Stretch:
      return "stretch";
    case ScalingMode::Crop:
      return "crop";
    case ScalingMode::Fit:
      return "fit";
    default:
      NOTREACHED();
      return "";
  }
}

void HTMLSkypeVideoElement::setScalingMode(const String& value) {
  LogEvent(kLogLevelDebug,
           base::StringPrintf("HTMLSkypeVideoElement::setScalingMode('%s')",
                              value.Utf8().data()));

  if (value == "stretch") {
    scaling_mode_ = ScalingMode::Stretch;
  } else if (value == "crop") {
    scaling_mode_ = ScalingMode::Crop;
  } else if (value == "fit") {
    scaling_mode_ = ScalingMode::Fit;
  }

  SetStyleProperty("object-fit", GetObjectFit());
  SetStyleProperty("object-position", GetObjectPosition());
}

bool HTMLSkypeVideoElement::bufferSharingEnabled() const {
  return buffer_sharing_enabled_;
}

void HTMLSkypeVideoElement::setBufferSharingEnabled(bool value) {
  LogEvent(
      kLogLevelDebug,
      base::StringPrintf("HTMLSkypeVideoElement::setBufferSharingEnabled(%s)",
                         value ? "true" : "false"));

  buffer_sharing_enabled_ = value;
}

Node::InsertionNotificationRequest HTMLSkypeVideoElement::InsertedInto(
    ContainerNode& node) {
  LogEvent(kLogLevelDebug, "HTMLSkypeVideoElement::InsertedInto");

  return HTMLVideoElement::InsertedInto(node);
}

void HTMLSkypeVideoElement::RemovedFrom(ContainerNode& node) {
  LogEvent(kLogLevelDebug, "HTMLSkypeVideoElement::RemovedFrom");

  return HTMLVideoElement::RemovedFrom(node);
}

void HTMLSkypeVideoElement::CheckVideoPosition() {
  SetStyleProperty("object-position", GetObjectPosition());
}

SkypeMediaPlayerClient* HTMLSkypeVideoElement::GetSkypeMediaPlayerClient() {
  return this;
}

bool HTMLSkypeVideoElement::UseBufferSharing() const {
  return buffer_sharing_enabled_;
}

static Event* CreateEventWithTarget(const AtomicString& event_name,
                                    EventTarget* event_target) {
  Event* event = Event::Create(event_name);
  event->SetTarget(event_target);
  return event;
}

void HTMLSkypeVideoElement::BackgroundRenderingChanged() {
  ScheduleEvent(CreateEventWithTarget(
      event_type_names::kMsBackgroundRenderingChanged, this));
}

void HTMLSkypeVideoElement::TextureSharingSupportedChanged() {
  ScheduleEvent(CreateEventWithTarget(
      event_type_names::kMsTextureSharingSupportedChanged, this));
}

void HTMLSkypeVideoElement::RendererSizeChanged() {
  ScheduleEvent(
      CreateEventWithTarget(event_type_names::kMsRendererSizeChanged, this));
}

void HTMLSkypeVideoElement::VideoPositionChanged() {
  CheckVideoPosition();
}

void HTMLSkypeVideoElement::LogEvent(SkypeMediaPlayerClient::LogLevel level,
                                     const std::string& message) {
  Event* event = LogEvent::Create(LogEvent::LogLevel(level),
                                  blink::WebString::FromUTF8(message));
  event->SetTarget(this);
  ScheduleEvent(event);
}

SkypeMediaPlayer* HTMLSkypeVideoElement::GetSkypeMediaPlayer() const {
  if (auto* player = GetWebMediaPlayer()) {
    return player->GetSkypeMediaPlayer();
  } else {
    return nullptr;
  }
}

String HTMLSkypeVideoElement::GetObjectFit() const {
  switch (scaling_mode_) {
    case ScalingMode::Stretch:
      return "fill";
    case ScalingMode::Crop:
      return "cover";
    case ScalingMode::Fit:
      return "contain";
    default:
      NOTREACHED();
      return "";
  }
}

String HTMLSkypeVideoElement::GetObjectPosition() const {
  auto* player = GetSkypeMediaPlayer();

  if (!player || scaling_mode_ != ScalingMode::Crop) {
    return "";
  }

  auto positionX = player->VideoPositionX();
  auto positionY = player->VideoPositionY();

  return String::Format("%.0f%% %.0f%%", positionX * 100, positionY * 100);
}

void HTMLSkypeVideoElement::SetStyleProperty(const String& name,
                                             const String& value) {
  style()->setProperty(GetExecutionContext(), name, value, "",
                       ASSERT_NO_EXCEPTION);
}

}  // namespace blink
