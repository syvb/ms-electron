// Copyright (c) 2020 Microsoft Corporation.

#ifndef SRC_ELECTRON_SHELL_RENDERER_RENDERER_SERVICE_H_
#define SRC_ELECTRON_SHELL_RENDERER_RENDERER_SERVICE_H_

#include <string>
#include <vector>

#include "mojo/public/cpp/bindings/self_owned_receiver.h"
#include "shell/common/api/api.mojom.h"
#include "shell/common/gin_helper/event_emitter.h"

namespace content {
class RenderFrame;
}

namespace electron {
class RendererClientBase;
}

namespace microsoft {

/**
 *  A simple utility class to do common event functionality
 */
class EventHelper {
 public:
  // Emits args to function identified by 'function_key', on 'ipcNative'
  // object.
  static void EmitIPCEvent(v8::Local<v8::Context> context,
                           const std::string& function_key,
                           std::vector<v8::Local<v8::Value>> argv);
};

/**
 * RendererService is the service implementation
 * that receives Sync and Async messages from a
 * connected rendererer.
 */
class RendererService : public electron::mojom::RendererService {
 public:
  RendererService(content::RenderFrame* render_frame,
                  electron::RendererClientBase* renderer_client);
  ~RendererService() override;

  void Message(bool internal,
               const std::string& channel,
               blink::CloneableMessage arguments,
               int sender_id) override;
  void MessageSync(bool internal,
                   const std::string& channel,
                   blink::CloneableMessage arguments,
                   int sender_id,
                   MessageSyncCallback callback) override;

 private:
  content::RenderFrame* render_frame_;
  electron::RendererClientBase* renderer_client_;
};

}  // namespace microsoft

#endif  // SRC_ELECTRON_SHELL_RENDERER_RENDERER_SERVICE_H_
