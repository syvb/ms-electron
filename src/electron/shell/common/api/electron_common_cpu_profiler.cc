// Copyright (c) 2020 Microsoft Corporation.

#include "microsoft/src/electron/shell/common/api/electron_common_cpu_profiler.h"

#include <vector>

#include "base/logging.h"
#include "base/optional.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/object_template_builder.h"
#include "shell/common/node_includes.h"

namespace {

base::Optional<std::vector<v8::Local<v8::Value>>> GetLineTicks(
    v8::Isolate* isolate,
    const v8::CpuProfileNode* node) {
  unsigned int count = node->GetHitLineCount();
  std::vector<v8::CpuProfileNode::LineTick> entries(count);

  if (!node->GetLineTicks(entries.data(), entries.size())) {
    return base::nullopt;
  }

  std::vector<v8::Local<v8::Value>> items(count);

  for (unsigned int index = 0; index < count; index++) {
    gin_helper::Dictionary dict = gin::Dictionary::CreateEmpty(isolate);
    dict.SetHidden("simple", true);
    dict.Set("line", entries[index].line);
    dict.Set("hitCount", entries[index].hit_count);

    items[index] = dict.GetHandle();
  }

  return items;
}

v8::Local<v8::Value> SerializeNode(v8::Isolate* isolate,
                                   const v8::CpuProfileNode* node) {
  gin_helper::Dictionary dict = gin::Dictionary::CreateEmpty(isolate);
  dict.SetHidden("simple", true);
  dict.Set("functionName", node->GetFunctionName());
  dict.Set("scriptId", node->GetScriptId());
  dict.Set("url", node->GetScriptResourceName());
  dict.Set("lineNumber", node->GetLineNumber());
  dict.Set("columnNumber", node->GetColumnNumber());
  dict.Set("bailoutReason", node->GetBailoutReason());
  dict.Set("hitCount", node->GetHitCount());
  dict.Set("id", node->GetNodeId());

  int count = node->GetChildrenCount();
  std::vector<v8::Local<v8::Value>> children(count);

  for (int index = 0; index < count; index++) {
    children[index] = SerializeNode(isolate, node->GetChild(index));
  }

  dict.Set("children", children);

  if (auto value = GetLineTicks(isolate, node)) {
    dict.Set("lineTicks", *value);
  }

  return dict.GetHandle();
}

v8::Local<v8::Value> SerializeProfile(v8::Isolate* isolate,
                                      v8::CpuProfile* profile) {
  gin_helper::Dictionary dict = gin::Dictionary::CreateEmpty(isolate);
  dict.SetHidden("simple", true);
  dict.Set("typeId", "CPU");
  dict.Set("title", profile->GetTitle());
  dict.Set("startTime", profile->GetStartTime() / 1000000);
  dict.Set("endTime", profile->GetEndTime() / 1000000);
  dict.Set("head", SerializeNode(isolate, profile->GetTopDownRoot()));

  int count = profile->GetSamplesCount();
  std::vector<unsigned> samples(count);
  std::vector<double> timestamps(count);

  for (int index = 0; index < count; index++) {
    samples[index] = profile->GetSample(index)->GetNodeId();
    timestamps[index] = static_cast<double>(profile->GetSampleTimestamp(index));
  }

  dict.Set("samples", samples);
  dict.Set("timestamps", timestamps);

  return dict.GetHandle();
}

}  // namespace

namespace microsoft {

CpuProfiler::CpuProfiler(gin::Arguments* args) {
  InitWithArgs(args);

  profiler_ = v8::CpuProfiler::New(args->isolate());
}

CpuProfiler::~CpuProfiler() {
  if (profiler_) {
    profiler_->Dispose();
  }
}

// static
gin_helper::WrappableBase* CpuProfiler::New(gin::Arguments* args) {
  return new CpuProfiler(args);
}

// static
void CpuProfiler::BuildPrototype(v8::Isolate* isolate,
                                 v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(gin::StringToV8(isolate, "CpuProfiler"));
  gin_helper::Destroyable::MakeDestroyable(isolate, prototype);
  gin_helper::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
      .SetMethod("startProfiling", &CpuProfiler::StartProfiling)
      .SetMethod("stopProfiling", &CpuProfiler::StopProfiling)
      .SetMethod("setSamplingInterval", &CpuProfiler::SetSamplingInterval)
      .SetMethod("setUsePreciseSampling", &CpuProfiler::SetUsePreciseSampling);
}

void CpuProfiler::StartProfiling(v8::Isolate* isolate,
                                 const std::string& title,
                                 bool record_samples) {
  DCHECK(profiler_);
  profiler_->StartProfiling(gin::StringToV8(isolate, title), record_samples);
}

v8::Local<v8::Value> CpuProfiler::StopProfiling(v8::Isolate* isolate,
                                                const std::string& title) {
  DCHECK(profiler_);
  auto* profile = profiler_->StopProfiling(gin::StringToV8(isolate, title));
  if (!profile) {
    return v8::Null(isolate);
  }

  auto result = SerializeProfile(isolate, profile);
  profile->Delete();
  return result;
}

void CpuProfiler::SetSamplingInterval(int us) {
  DCHECK(profiler_);
  profiler_->SetSamplingInterval(us);
}

void CpuProfiler::SetUsePreciseSampling(bool value) {
  DCHECK(profiler_);
  profiler_->SetUsePreciseSampling(value);
}

}  // namespace microsoft

namespace {

using microsoft::CpuProfiler;

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  CpuProfiler::SetConstructor(isolate, base::BindRepeating(&CpuProfiler::New));

  gin_helper::Dictionary dict(isolate, exports);
  dict.Set("CpuProfiler", CpuProfiler::GetConstructor(isolate)
                              ->GetFunction(context)
                              .ToLocalChecked());
}

}  // namespace

NODE_LINKED_MODULE_CONTEXT_AWARE(electron_common_cpu_profiler, Initialize)
