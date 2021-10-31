// Copyright (c) 2019 Microsoft Corporation.

#include "microsoft/src/electron/shell/node_bindings_ms.h"

#include "microsoft/buildflags/buildflags.h"

void _register_electron_common_cpu_profiler();

namespace microsoft {

void RegisterBuiltinModules() {
#if BUILDFLAG(MICROSOFT_ENABLE_CPU_PROFILER)
  _register_electron_common_cpu_profiler();
#endif
}

}  // namespace microsoft
