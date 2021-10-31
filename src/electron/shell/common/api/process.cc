// Copyright (c) 2020 Microsoft Corporation.

#include "microsoft/src/electron/shell/common/api/process.h"

#include "microsoft/buildflags/buildflags.h"

#if BUILDFLAG(MICROSOFT_ENABLE_MEMORY_USAGE_API)
#include "microsoft/src/electron/shell/common/memory_info.h"
#endif

#if BUILDFLAG(MICROSOFT_ENABLE_STARTUP_METRICS)
#include "microsoft/src/electron/shell/common/startup_metrics_v8.h"
#endif

#if BUILDFLAG(MICROSOFT_ENABLE_WATCHDOG)
#include "microsoft/src/electron/shell/common/api/watchdog.h"
#endif

#if BUILDFLAG(MICROSOFT_ENABLE_WER_API) && defined(OS_WIN)
#include "microsoft/src/electron/shell/common/wer_api_win.h"
#endif

namespace microsoft {

void ExtendProcessObject(gin_helper::Dictionary* process) {
#if BUILDFLAG(MICROSOFT_ENABLE_MEMORY_USAGE_API)
  process->SetMethod("getProcessMemoryInfoEx",
                     &microsoft::GetProcessMemoryInfoEx);
#if defined(OS_WIN)
  process->SetMethod("getMallocMemoryUsage", &microsoft::GetMallocMemoryUsage);
  process->SetMethod("getLoadedModulesSize", &microsoft::GetLoadedModulesSize);
#endif
#endif

#if BUILDFLAG(MICROSOFT_ENABLE_STARTUP_METRICS)
  process->SetMethod("getStartupMetrics", &microsoft::GetStartupMetrics);
#endif

#if BUILDFLAG(MICROSOFT_ENABLE_WATCHDOG)
  process->SetMethod("startWatchdog", &microsoft::Watchdog::Start);
  process->SetMethod("stopWatchdog", &microsoft::Watchdog::Stop);
#endif

#if BUILDFLAG(MICROSOFT_ENABLE_WER_API) && defined(OS_WIN)
  process->SetMethod("werInitialize", &microsoft::WerInitialize);
  process->SetMethod("werRegisterFile", &microsoft::WerRegisterFile);
  process->SetMethod("werUnregisterFile", &microsoft::WerUnregisterFile);
  process->SetMethod("werRegisterCustomMetadata",
                     &microsoft::WerRegisterCustomMetadata);
  process->SetMethod("werUnregisterCustomMetadata",
                     &microsoft::WerUnregisterCustomMetadata);
#endif
}

}  // namespace microsoft
