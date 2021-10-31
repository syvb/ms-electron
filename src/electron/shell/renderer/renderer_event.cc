// Copyright (c) 2021 Microsoft Corporation.

#include "microsoft/src/electron/shell/renderer/renderer_event.h"

#include <utility>

#include "shell/common/gin_converters/blink_converter.h"
#include "shell/common/gin_converters/value_converter.h"

namespace microsoft {

// static
gin::WrapperInfo RendererEvent::kWrapperInfo = {gin::kEmbedderNativeGin};

// static
gin::Handle<RendererEvent> RendererEvent::Create(
    v8::Isolate* isolate,
    int32_t sender_id,
    bool direct_channel,
    electron::mojom::ElectronRenderer::MessageSyncCallback callback) {
  return gin::CreateHandle(isolate, new RendererEvent(sender_id, direct_channel,
                                                      std::move(callback)));
}

gin::ObjectTemplateBuilder RendererEvent::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return gin::Wrappable<RendererEvent>::GetObjectTemplateBuilder(isolate)
      .SetProperty("senderId", &RendererEvent::sender_id)
      .SetProperty("isDirectChannel", &RendererEvent::direct_channel)
      .SetMethod("sendReply", &RendererEvent::SendReply);
}

RendererEvent::RendererEvent(
    int32_t sender_id,
    bool direct_channel,
    electron::mojom::ElectronRenderer::MessageSyncCallback callback)
    : sender_id_(sender_id),
      direct_channel_(direct_channel),
      callback_(std::move(callback)) {}

RendererEvent::~RendererEvent() {
  if (callback_) {
    std::move(*callback_).Run(blink::CloneableMessage(), false);
  }
}

bool RendererEvent::SendReply(v8::Isolate* isolate,
                              v8::Local<v8::Value> result) {
  if (!callback_)
    return false;

  blink::CloneableMessage message;
  if (!gin::ConvertFromV8(isolate, result, &message)) {
    LOG(ERROR) << "Unable to convert reply message";
    return false;
  }

  std::move(*callback_).Run(std::move(message), true);
  callback_.reset();
  return true;
}

}  // namespace microsoft
