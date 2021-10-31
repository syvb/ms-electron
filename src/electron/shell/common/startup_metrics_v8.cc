// Copyright (c) 2020 Microsoft Corporation.

#include "microsoft/src/electron/shell/common/startup_metrics_v8.h"

#include "microsoft/src/electron/shell/common/startup_metrics.h"
#include "shell/common/gin_helper/dictionary.h"

namespace gin {

template <>
struct Converter<microsoft::StartupMetric> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const microsoft::StartupMetric& metric) {
    if (auto value = metric.GetValue()) {
      return v8::Number::New(isolate, *value);
    } else {
      return v8::Undefined(isolate);
    }
  }
};

}  // namespace gin

namespace microsoft {

v8::Local<v8::Value> GetStartupMetrics(v8::Isolate* isolate) {
  gin_helper::Dictionary dict = gin::Dictionary::CreateEmpty(isolate);
  dict.SetHidden("simple", true);
  dict.Set("mainTimestamp", startup_metrics::main);
  dict.Set("basicStartupComplete", startup_metrics::basic_startup_complete);
  dict.Set("renderThreadStarted", startup_metrics::render_thread_started);
  dict.Set("renderFrameCreated", startup_metrics::render_frame_created);
  dict.Set("didCreateScriptContext",
           startup_metrics::did_create_script_context);
  dict.Set("injectCrashReportingBefore",
           startup_metrics::inject_crash_reporting_before);
  dict.Set("injectCrashReportingAfter",
           startup_metrics::inject_crash_reporting_after);
  dict.Set("initCrashReportingBefore",
           startup_metrics::init_crash_reporting_before);
  dict.Set("initCrashReportingAfter",
           startup_metrics::init_crash_reporting_after);
  dict.Set("nodeLoadEnvironment", startup_metrics::node_load_environment);
  dict.Set("initStart", startup_metrics::init_start);
  dict.Set("initFinish", startup_metrics::init_finish);

  return dict.GetHandle();
}

}  // namespace microsoft
