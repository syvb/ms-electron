// Copyright (c) 2020 Microsoft Corporation.

#include "microsoft/src/electron/shell/renderer/renderer_service.h"

#include <memory>
#include <utility>

#include "content/public/renderer/render_frame.h"
#include "microsoft/src/electron/shell/renderer/renderer_event.h"
#include "shell/common/gin_converters/blink_converter.h"
#include "shell/common/gin_converters/value_converter.h"
#include "shell/common/node_includes.h"
#include "shell/renderer/renderer_client_base.h"
#include "third_party/blink/public/web/blink.h"
#include "third_party/blink/public/web/web_local_frame.h"

// FIXME (Ravi): Common code copied. Need some place to keep a single copy
namespace {

const char kIpcKey[] = "ipcNative";

// Gets the private object under kIpcKey
v8::Local<v8::Object> GetIpcObject(v8::Local<v8::Context> context) {
  auto* isolate = context->GetIsolate();
  auto binding_key = gin::StringToV8(isolate, kIpcKey);
  auto private_binding_key = v8::Private::ForApi(isolate, binding_key);
  auto global_object = context->Global();
  auto value =
      global_object->GetPrivate(context, private_binding_key).ToLocalChecked();
  if (value.IsEmpty() || !value->IsObject()) {
    LOG(ERROR) << "Attempted to get the 'ipcNative' object but it was missing";
    return v8::Local<v8::Object>();
  }
  return value->ToObject(context).ToLocalChecked();
}

void InvokeIpcCallback(v8::Local<v8::Context> context,
                       const std::string& callback_name,
                       std::vector<v8::Local<v8::Value>> args) {
  TRACE_EVENT0("devtools.timeline", "FunctionCall");
  auto* isolate = context->GetIsolate();

  auto ipcNative = GetIpcObject(context);
  if (ipcNative.IsEmpty())
    return;

  // Only set up the node::CallbackScope if there's a node environment.
  // Sandboxed renderers don't have a node environment.
  node::Environment* env = node::Environment::GetCurrent(context);
  std::unique_ptr<node::CallbackScope> callback_scope;
  if (env) {
    callback_scope.reset(new node::CallbackScope(isolate, ipcNative, {0, 0}));
  }

  auto callback_key = gin::ConvertToV8(isolate, callback_name)
                          ->ToString(context)
                          .ToLocalChecked();
  auto callback_value = ipcNative->Get(context, callback_key).ToLocalChecked();
  DCHECK(callback_value->IsFunction());  // set by init.ts
  auto callback = callback_value.As<v8::Function>();
  ignore_result(callback->Call(context, ipcNative, args.size(), args.data()));
}

}  // namespace

namespace microsoft {

/* static */
void EventHelper::EmitIPCEvent(v8::Local<v8::Context> context,
                               const std::string& function_key,
                               std::vector<v8::Local<v8::Value>> argv) {
  auto* isolate = context->GetIsolate();
  v8::HandleScope handle_scope(isolate);
  v8::Context::Scope context_scope(context);
  v8::MicrotasksScope script_scope(isolate,
                                   v8::MicrotasksScope::kRunMicrotasks);
  InvokeIpcCallback(context, function_key, std::move(argv));
}

// RendererService Implementation
RendererService::RendererService(content::RenderFrame* render_frame,
                                 electron::RendererClientBase* renderer_client)
    : render_frame_(render_frame), renderer_client_(renderer_client) {
  DCHECK(renderer_client);
  DCHECK(render_frame);
}

RendererService::~RendererService() = default;

void RendererService::Message(bool internal,
                              const std::string& channel,
                              blink::CloneableMessage arguments,
                              int sender_id) {
  blink::WebLocalFrame* frame = render_frame_->GetWebFrame();
  if (!frame)
    return;

  v8::Isolate* isolate = blink::MainThreadIsolate();
  v8::HandleScope handle_scope(isolate);

  v8::Local<v8::Context> context = renderer_client_->GetContext(frame, isolate);
  v8::Context::Scope context_scope(context);

  std::vector<v8::Local<v8::Value>> ports;
  std::vector<v8::Local<v8::Value>> argv = {
      gin::ConvertToV8(isolate, internal),
      gin::ConvertToV8(isolate, channel),
      gin::ConvertToV8(isolate, ports),
      gin::ConvertToV8(isolate, arguments),
      gin::ConvertToV8(isolate, sender_id),
      gin::ConvertToV8(isolate, true)};

  EventHelper::EmitIPCEvent(context, "onMessage", std::move(argv));
}

void RendererService::MessageSync(bool internal,
                                  const std::string& channel,
                                  blink::CloneableMessage arguments,
                                  int sender_id,
                                  MessageSyncCallback callback) {
  blink::WebLocalFrame* frame = render_frame_->GetWebFrame();
  // FIXME (Ravi): Do we need to make a callback with error message
  if (!frame) {
    return;
  }
  v8::Isolate* isolate = blink::MainThreadIsolate();
  v8::HandleScope handle_scope(isolate);

  v8::Local<v8::Context> context = renderer_client_->GetContext(frame, isolate);
  v8::Context::Scope context_scope(context);

  // FIXME(Ravi): Is this Calling Event leaking... ?
  // Check when we do a forced GC. Hopefully this will go away
  auto native_event =
      RendererEvent::Create(isolate, sender_id, true, std::move(callback));
  auto event = v8::Local<v8::Object>::Cast(native_event.ToV8());
  std::vector<v8::Local<v8::Value>> argv = {
      gin::ConvertToV8(isolate, internal), gin::ConvertToV8(isolate, channel),
      gin::ConvertToV8(isolate, arguments), gin::ConvertToV8(isolate, event)};

  EventHelper::EmitIPCEvent(context, "onMessageSync", std::move(argv));
}

}  // namespace microsoft
