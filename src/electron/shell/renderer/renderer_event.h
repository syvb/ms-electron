// Copyright (c) 2020 Microsoft Corporation.

#ifndef SRC_ELECTRON_SHELL_RENDERER_RENDERER_EVENT_H_
#define SRC_ELECTRON_SHELL_RENDERER_RENDERER_EVENT_H_

#include "base/optional.h"
#include "gin/handle.h"
#include "gin/object_template_builder.h"
#include "gin/public/wrapper_info.h"
#include "gin/wrappable.h"
#include "shell/common/api/api.mojom.h"

namespace microsoft {

class RendererEvent : public gin::Wrappable<RendererEvent> {
 public:
  static gin::WrapperInfo kWrapperInfo;
  static gin::Handle<RendererEvent> Create(
      v8::Isolate* isolate,
      int32_t sender_id,
      bool direct_channel,
      electron::mojom::ElectronRenderer::MessageSyncCallback callback);

  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;

  ~RendererEvent() override;

 private:
  explicit RendererEvent(
      int32_t sender_id,
      bool direct_channel,
      electron::mojom::ElectronRenderer::MessageSyncCallback callback);

  int32_t sender_id() const { return sender_id_; }
  bool direct_channel() const { return direct_channel_; }

  bool SendReply(v8::Isolate* isolate, v8::Local<v8::Value> result);

  int32_t sender_id_ = 0;
  bool direct_channel_ = false;
  base::Optional<electron::mojom::ElectronRenderer::MessageSyncCallback>
      callback_;
};

}  // namespace microsoft

#endif  // SRC_ELECTRON_SHELL_RENDERER_RENDERER_EVENT_H_
