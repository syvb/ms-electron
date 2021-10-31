// Copyright (c) 2020 Microsoft Corporation.

#ifndef SRC_ELECTRON_SHELL_RENDERER_DIRECT_CONNECTION_MANAGER_H_
#define SRC_ELECTRON_SHELL_RENDERER_DIRECT_CONNECTION_MANAGER_H_

#include <string>
#include <unordered_map>

#include "gin/handle.h"
#include "gin/wrappable.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "shell/common/api/api.mojom.h"
#include "shell/common/gin_helper/error_thrower.h"
#include "shell/common/gin_helper/event_emitter.h"
#include "third_party/blink/public/web/blink.h"

namespace content {
class RenderFrame;
}

namespace microsoft {
/**
 * DirectConnectionManager manages direct mojo based
 * connection between Renderers. The renderers
 * are identified by their web_contents_id. This class
 * has be owned by a RenderFrame Observer and needs
 * an electron_browser_ptr (browser main process side service) to
 * broker initial connection between the renderers.
 * This class is intended to be used by MainFrames of renderer.
 * IMPORTANT: All methods are expected to run on Main thread.
 */
class DirectConnectionManager {
 public:
  explicit DirectConnectionManager(content::RenderFrame* render_frame);
  ~DirectConnectionManager();

  void ConnectToRenderer(
      const mojo::Remote<electron::mojom::ElectronBrowser>& browser_ptr,
      int32_t target_web_contents_id);
  void Close();

  v8::Local<v8::Value> SendToRendererSync(
      v8::Isolate* isolate,
      gin_helper::ErrorThrower thrower,
      bool internal,
      int32_t web_contents_id,
      const std::string& channel,
      v8::Local<v8::Value> arguments,
      const mojo::Remote<electron::mojom::ElectronBrowser>&
          electron_browser_remote);

  void SendToRenderer(v8::Isolate* isolate,
                      gin_helper::ErrorThrower thrower,
                      bool internal,
                      int32_t web_contents_id,
                      const std::string& channel,
                      v8::Local<v8::Value> arguments,
                      const mojo::Remote<electron::mojom::ElectronBrowser>&
                          electron_browser_remote);

 private:
  // Get the context that the Electron API is running in.
  v8::Local<v8::Context> GetContext(blink::WebLocalFrame* frame,
                                    v8::Isolate* isolate) const;
  // Emits any connection releated events
  void EmitConnectionEvent(int web_contents_id, bool connected);
  // Listener for any connection errors
  void OnConnectionError(int32_t web_contents_id);
  void OnRendererConnected(int32_t target_web_contents_id, int32_t sender_id);
  void RegisterConnection(
      int32_t web_contents_id,
      mojo::Remote<electron::mojom::RendererService> remote_render_service);
  // render_frame_ should be valid as the owning class is RenderFrameObserver
  content::RenderFrame* render_frame_;
  // map of web_contents_id and remote connections (aka mojo bound)
  std::unordered_map<int32_t, mojo::Remote<electron::mojom::RendererService>>
      renderer_services_;
  // map of web_contents_id and pending remote connections.
  std::unordered_map<int32_t, mojo::Remote<electron::mojom::RendererService>>
      pending_renderer_services_;
  // This renderers web_contents_id. Will be populated on first connect.
  int sender_id_ = 0;
  bool isolated_world_ = false;
};

}  // namespace microsoft

#endif  // SRC_ELECTRON_SHELL_RENDERER_DIRECT_CONNECTION_MANAGER_H_
