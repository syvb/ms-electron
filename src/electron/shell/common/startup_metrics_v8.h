// Copyright (c) 2020 Microsoft Corporation.

#ifndef SRC_ELECTRON_SHELL_COMMON_STARTUP_METRICS_V8_H_
#define SRC_ELECTRON_SHELL_COMMON_STARTUP_METRICS_V8_H_

#include "v8/include/v8.h"

namespace microsoft {

v8::Local<v8::Value> GetStartupMetrics(v8::Isolate* isolate);

}  // namespace microsoft

#endif  // SRC_ELECTRON_SHELL_COMMON_STARTUP_METRICS_V8_H_
