// Copyright (c) 2019 Microsoft Corporation.

#include "microsoft/src/electron/shell/common/memory_info.h"

#include <vector>

#include "base/optional.h"
#include "shell/common/gin_helper/dictionary.h"

#if defined(OS_WIN)
#include <windows.h>

#include <malloc.h>
#include <psapi.h>
#include "base/win/win_util.h"
#endif

#if defined(OS_MACOSX)
#include <mach/mach.h>

namespace {

base::Optional<mach_task_basic_info_data_t> GetTaskInfo(mach_port_t task) {
  if (task == MACH_PORT_NULL)
    return base::nullopt;
  mach_task_basic_info_data_t info = {};
  mach_msg_type_number_t count = MACH_TASK_BASIC_INFO_COUNT;
  kern_return_t kr = task_info(task, MACH_TASK_BASIC_INFO,
                               reinterpret_cast<task_info_t>(&info), &count);
  return (kr == KERN_SUCCESS) ? base::make_optional(info) : base::nullopt;
}

}  // namespace

#endif  // defined(OS_MACOSX)

namespace microsoft {

#if defined(OS_WIN)

v8::Local<v8::Value> GetProcessMemoryInfoEx(v8::Isolate* isolate) {
  size_t working_set_size = 0;
  size_t peak_working_set_size = 0;
  size_t private_bytes = 0;

  PROCESS_MEMORY_COUNTERS_EX info = {};
  if (GetProcessMemoryInfo(GetCurrentProcess(),
                           reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&info),
                           sizeof(info))) {
    working_set_size = info.WorkingSetSize;
    peak_working_set_size = info.PeakWorkingSetSize;
    private_bytes = info.PrivateUsage;
  }

  gin_helper::Dictionary dict = gin::Dictionary::CreateEmpty(isolate);
  dict.SetHidden("simple", true);
  dict.Set("workingSetSize", static_cast<double>(working_set_size >> 10));
  dict.Set("peakWorkingSetSize",
           static_cast<double>(peak_working_set_size >> 10));
  dict.Set("privateBytes", static_cast<double>(private_bytes >> 10));

  return dict.GetHandle();
}

v8::Local<v8::Value> GetMallocMemoryUsage(v8::Isolate* isolate,
                                          uint64_t maxSize,
                                          uint64_t maxCount,
                                          uint64_t maxTimeToRun) {
  uint64_t committedSize = 0;
  uint64_t uncommittedSize = 0;
  uint64_t allocatedObjectsSize = 0;
  uint64_t allocatedObjectsCount = 0;
  HANDLE crt_heap = reinterpret_cast<HANDLE>(_get_heap_handle());
  HeapLock(crt_heap);
  PROCESS_HEAP_ENTRY heap_entry = {};

  uint64_t endTime = GetTickCount64() + maxTimeToRun;
  bool isDataComplete = true;

  uint64_t iterations = 0;
  while (HeapWalk(crt_heap, &heap_entry) != FALSE) {
    if (heap_entry.wFlags & PROCESS_HEAP_ENTRY_BUSY) {
      allocatedObjectsSize += heap_entry.cbData;
      allocatedObjectsCount++;
    } else if (heap_entry.wFlags & PROCESS_HEAP_REGION) {
      committedSize += heap_entry.Region.dwCommittedSize;
      uncommittedSize += heap_entry.Region.dwUnCommittedSize;
    }

    if (allocatedObjectsSize >= maxSize || allocatedObjectsCount >= maxCount ||
        (iterations % 1000 == 0 && GetTickCount64() >= endTime)) {
      isDataComplete = false;
      break;
    }
    ++iterations;
  }
  HeapUnlock(crt_heap);

  gin_helper::Dictionary dict = gin::Dictionary::CreateEmpty(isolate);
  dict.SetHidden("simple", true);
  dict.Set("committedSize", (committedSize >> 10));
  dict.Set("uncommittedSize", (uncommittedSize >> 10));
  dict.Set("allocatedObjectsSize", (allocatedObjectsSize >> 10));
  dict.Set("allocatedObjectsCount", allocatedObjectsCount);
  dict.Set("isDataComplete", isDataComplete);

  return dict.GetHandle();
}

double GetLoadedModulesSize(v8::Isolate* isolate) {
  uint64_t loadedModulesSize = 0;

  auto* hProcess = GetCurrentProcess();
  std::vector<HMODULE> modules;
  if (base::win::GetLoadedModulesSnapshot(hProcess, &modules)) {
    for (auto* hModule : modules) {
      MODULEINFO mi;
      if (GetModuleInformation(hProcess, hModule, &mi, sizeof(mi))) {
        loadedModulesSize += mi.SizeOfImage;
      }
    }
  }

  // we want to keep the precision of this number hence, don't convert to
  // kilobytes
  return static_cast<double>(loadedModulesSize);
}

#else  // !defined(OS_WIN)

v8::Local<v8::Value> GetProcessMemoryInfoEx(v8::Isolate* isolate) {
  size_t working_set_size = 0;
  size_t peak_working_set_size = 0;

#if defined(OS_MACOSX)
  if (auto info = GetTaskInfo(mach_task_self())) {
    working_set_size = info->resident_size;
    peak_working_set_size = info->resident_size_max;
  }
#endif

  gin_helper::Dictionary dict = gin::Dictionary::CreateEmpty(isolate);
  dict.SetHidden("simple", true);
  dict.Set("workingSetSize", static_cast<double>(working_set_size >> 10));
  dict.Set("peakWorkingSetSize",
           static_cast<double>(peak_working_set_size >> 10));

  return dict.GetHandle();
}

#endif  // !defined(OS_WIN)

}  // namespace microsoft
