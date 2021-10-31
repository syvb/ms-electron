// Copyright (c) 2019 Microsoft Corporation.

#ifndef SRC_ELECTRON_SHELL_COMMON_STARTUP_METRICS_H_
#define SRC_ELECTRON_SHELL_COMMON_STARTUP_METRICS_H_

#include "base/optional.h"
#include "base/time/time.h"

namespace microsoft {

class StartupMetric {
 public:
  StartupMetric();

  void Mark();
  void Mark(double timestamp);

  base::Optional<double> GetValue() const { return value_; }

 private:
  base::Optional<double> value_;
};

namespace startup_metrics {

extern StartupMetric main;
extern StartupMetric basic_startup_complete;
extern StartupMetric render_thread_started;
extern StartupMetric render_frame_created;
extern StartupMetric did_create_script_context;
extern StartupMetric init_crash_reporting_before;
extern StartupMetric init_crash_reporting_after;
extern StartupMetric inject_crash_reporting_before;
extern StartupMetric inject_crash_reporting_after;
extern StartupMetric node_load_environment;
extern StartupMetric init_start;
extern StartupMetric init_finish;

}  // namespace startup_metrics

}  // namespace microsoft

#endif  // SRC_ELECTRON_SHELL_COMMON_STARTUP_METRICS_H_
