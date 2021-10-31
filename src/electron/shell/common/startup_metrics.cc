// Copyright (c) 2019 Microsoft Corporation.

#include "microsoft/src/electron/shell/common/startup_metrics.h"

namespace microsoft {

StartupMetric::StartupMetric() = default;

void StartupMetric::Mark() {
  Mark(base::Time::Now().ToJsTime());
}

void StartupMetric::Mark(double timestamp) {
  value_ = timestamp - startup_metrics::main.value_.value_or(0);
}

namespace startup_metrics {

StartupMetric main;
StartupMetric basic_startup_complete;
StartupMetric render_thread_started;
StartupMetric render_frame_created;
StartupMetric did_create_script_context;
StartupMetric init_crash_reporting_before;
StartupMetric init_crash_reporting_after;
StartupMetric inject_crash_reporting_before;
StartupMetric inject_crash_reporting_after;
StartupMetric node_load_environment;
StartupMetric init_start;
StartupMetric init_finish;

}  // namespace startup_metrics

}  // namespace microsoft
