// Copyright (c) 2020 Microsoft Corporation.

#include "microsoft/src/electron/shell/renderer/direct_connection_manager.h"

#include <string>
#include <utility>
#include <vector>

#include "base/command_line.h"
#include "content/public/renderer/render_frame.h"
#include "microsoft/src/electron/shell/renderer/renderer_service.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "shell/common/gin_helper/event_emitter.h"
#include "shell/common/node_includes.h"
#include "shell/common/options_switches.h"
#include "shell/common/v8_value_serializer.h"
#include "shell/common/world_ids.h"
#include "shell/renderer/electron_render_frame_observer.h"
#include "third_party/blink/public/web/blink.h"
#include "third_party/blink/public/web/web_local_frame.h"

namespace microsoft {

DirectConnectionManager::DirectConnectionManager(
    content::RenderFrame* render_frame)
    : render_frame_(render_frame) {
  DCHECK(render_frame_);

  isolated_world_ = base::CommandLine::ForCurrentProcess()->HasSwitch(
      electron::switches::kContextIsolation);
}

DirectConnectionManager::~DirectConnectionManager() {
  Close();
}

void DirectConnectionManager::OnConnectionError(int32_t web_contents_id) {
  LOG(INFO) << "Service disconnected for: " << web_contents_id;
  auto found = renderer_services_.find(web_contents_id);
  if (found == renderer_services_.end()) {
    // This should never happen.
    LOG(INFO) << "Renderer Service not found for: " << web_contents_id;
    return;
  }
  renderer_services_.erase(found);
  EmitConnectionEvent(web_contents_id, false);
}

void DirectConnectionManager::Close() {
  LOG(INFO) << "Closing all connections";
  renderer_services_.clear();
}

v8::Local<v8::Context> DirectConnectionManager::GetContext(
    blink::WebLocalFrame* frame,
    v8::Isolate* isolate) const {
  if (isolated_world_)
    return frame->WorldScriptContext(isolate,
                                     electron::WorldIDs::ISOLATED_WORLD_ID);
  else
    return frame->MainWorldScriptContext();
}

void DirectConnectionManager::EmitConnectionEvent(int web_contents_id,
                                                  bool connected) {
  LOG(INFO) << "EmitConnectionEvent - web_contents_id: " << web_contents_id
            << ", connected: " << connected;
  v8::Isolate* isolate = blink::MainThreadIsolate();
  v8::HandleScope handle_scope(isolate);

  v8::Local<v8::Context> context =
      GetContext(render_frame_->GetWebFrame(), isolate);
  v8::Context::Scope context_scope(context);

  std::vector<v8::Local<v8::Value>> argv = {
      gin::ConvertToV8(isolate, web_contents_id),
      gin::ConvertToV8(isolate, connected),
  };

  EventHelper::EmitIPCEvent(context, "onConnectStatus", std::move(argv));
}

void DirectConnectionManager::RegisterConnection(
    int32_t web_contents_id,
    mojo::Remote<electron::mojom::RendererService> remote_render_service) {
  // check we have a bound service, else no point going further
  DCHECK(remote_render_service.is_bound());
  auto found = renderer_services_.find(web_contents_id);
  if (found != renderer_services_.end()) {
    LOG(INFO) << "Found an already bound service for: " << web_contents_id;
    // replace that with a new one.
    renderer_services_.erase(found);
  }

  // It is safe to use base::Unretained(this) here since,
  // DirectConnectionManager owns all the remote connections and will outlive
  // them.
  auto connection_error_callback =
      base::BindOnce(&DirectConnectionManager::OnConnectionError,
                     base::Unretained(this), web_contents_id);
  remote_render_service.set_disconnect_handler(
      std::move(connection_error_callback));
  renderer_services_.emplace(web_contents_id, std::move(remote_render_service));
  EmitConnectionEvent(web_contents_id, true);
}

void DirectConnectionManager::OnRendererConnected(
    int32_t target_web_contents_id,
    int32_t sender_id) {
  auto found = pending_renderer_services_.find(target_web_contents_id);
  if (found == pending_renderer_services_.end()) {
    LOG(ERROR) << "Received OnRendererConnected for an unknown (not on pending "
                  "list) WebContents with id: "
               << target_web_contents_id;
    return;
  }

  // A non-zero sender_id is indicative of successful connection
  if (sender_id) {
    LOG(INFO) << "Successfully connected to renderer: "
              << target_web_contents_id
              << " sender WebContents id: " << sender_id;
    if (!sender_id_) {
      sender_id_ = sender_id;
    }
    RegisterConnection(target_web_contents_id, std::move(found->second));
  } else {
    // Send an error event if we are not successful
    EmitConnectionEvent(target_web_contents_id, false);
  }
  pending_renderer_services_.erase(found);
}

void DirectConnectionManager::ConnectToRenderer(
    const mojo::Remote<electron::mojom::ElectronBrowser>& electron_browser_ptr,
    int32_t target_web_contents_id) {
  // Check if we have already connected
  if (renderer_services_.find(target_web_contents_id) !=
      renderer_services_.end()) {
    LOG(ERROR) << "Target renderer is already connected: "
               << target_web_contents_id;
    return;
  }

  // Check if we are pending connected
  if (pending_renderer_services_.find(target_web_contents_id) !=
      pending_renderer_services_.end()) {
    LOG(ERROR) << "Target renderer is on pending connect list: "
               << target_web_contents_id;
    return;
  }

  LOG(INFO) << "Trying to connect to renderer with WebContents id: "
            << target_web_contents_id;

  mojo::Remote<electron::mojom::RendererService> remote_render_service;
  auto cb = base::BindOnce(&DirectConnectionManager::OnRendererConnected,
                           base::Unretained(this), target_web_contents_id);
  electron_browser_ptr->ConnectToRenderer(
      target_web_contents_id,
      remote_render_service.BindNewPipeAndPassReceiver(), std::move(cb));

  // Keep a pending remote open, until we get a confirmation of connection.
  pending_renderer_services_.emplace(target_web_contents_id,
                                     std::move(remote_render_service));
}

v8::Local<v8::Value> DirectConnectionManager::SendToRendererSync(
    v8::Isolate* isolate,
    gin_helper::ErrorThrower thrower,
    bool internal,
    int32_t web_contents_id,
    const std::string& channel,
    v8::Local<v8::Value> arguments,
    const mojo::Remote<electron::mojom::ElectronBrowser>&
        electron_browser_remote) {
  blink::CloneableMessage message;
  if (!electron::SerializeV8Value(isolate, arguments, &message)) {
    return v8::Local<v8::Value>();
  }

  blink::CloneableMessage result;

  auto found = renderer_services_.find(web_contents_id);
  if (found != renderer_services_.end()) {
    // send directly to the renderer
    bool success = false;
    found->second->MessageSync(internal, channel, std::move(message),
                               sender_id_, &result, &success);

    if (!success) {
      thrower.ThrowError("mojom::RendererService::MessageSync() failed");
    }
  } else {
    // proxy via the main process
    std::string error;
    electron_browser_remote->MessageToSync(internal, web_contents_id, channel,
                                           std::move(message), &error, &result);
    if (!error.empty()) {
      thrower.ThrowError(error);
    }
  }

  return electron::DeserializeV8Value(isolate, result);
}

void DirectConnectionManager::SendToRenderer(
    v8::Isolate* isolate,
    gin_helper::ErrorThrower thrower,
    bool internal,
    int32_t web_contents_id,
    const std::string& channel,
    v8::Local<v8::Value> arguments,
    const mojo::Remote<electron::mojom::ElectronBrowser>&
        electron_browser_remote) {
  blink::CloneableMessage message;
  if (!electron::SerializeV8Value(isolate, arguments, &message)) {
    return;
  }

  auto found = renderer_services_.find(web_contents_id);
  if (found != renderer_services_.end()) {
    // send directly to the renderer
    found->second->Message(internal, channel, std::move(message), sender_id_);
  } else {
    // proxy via the main process
    electron_browser_remote->MessageTo(internal,
                                       false
                                       /* send_to_all */,
                                       web_contents_id, channel,
                                       std::move(message));
  }
}

}  // namespace microsoft
