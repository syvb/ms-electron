// Copyright (c) 2020 Microsoft Corporation.

#ifndef SRC_ELECTRON_SHELL_COMMON_API_ELECTRON_COMMON_CPU_PROFILER_H_
#define SRC_ELECTRON_SHELL_COMMON_API_ELECTRON_COMMON_CPU_PROFILER_H_

#include <string>

#include "shell/common/gin_helper/trackable_object.h"
#include "v8/include/v8-profiler.h"

namespace microsoft {

class CpuProfiler : public gin_helper::TrackableObject<CpuProfiler> {
 public:
  static gin_helper::WrappableBase* New(gin::Arguments* args);

  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::FunctionTemplate> prototype);

 protected:
  explicit CpuProfiler(gin::Arguments* args);
  ~CpuProfiler() override;

 private:
  void StartProfiling(v8::Isolate* isolate,
                      const std::string& title,
                      bool record_samples);
  v8::Local<v8::Value> StopProfiling(v8::Isolate* isolate,
                                     const std::string& title);
  void SetSamplingInterval(int us);
  void SetUsePreciseSampling(bool value);

  v8::CpuProfiler* profiler_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(CpuProfiler);
};

}  // namespace microsoft

#endif  // SRC_ELECTRON_SHELL_COMMON_API_ELECTRON_COMMON_CPU_PROFILER_H_
