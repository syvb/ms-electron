// Copyright (c) 2019 Microsoft Corporation.

#ifndef SRC_ELECTRON_SHELL_COMMON_MEMORY_INFO_H_
#define SRC_ELECTRON_SHELL_COMMON_MEMORY_INFO_H_

#include "base/process/process_handle.h"
#include "v8/include/v8.h"

namespace microsoft {

v8::Local<v8::Value> GetProcessMemoryInfoEx(v8::Isolate* isolate);
#if defined(OS_WIN)
v8::Local<v8::Value> GetMallocMemoryUsage(v8::Isolate* isolate,
                                          uint64_t maxSize,
                                          uint64_t maxCount,
                                          uint64_t maxTimeToRun);
double GetLoadedModulesSize(v8::Isolate* isolate);
#endif

}  // namespace microsoft

#endif  // SRC_ELECTRON_SHELL_COMMON_MEMORY_INFO_H_
