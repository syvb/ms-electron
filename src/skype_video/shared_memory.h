// Copyright (c) 2019 Microsoft Corporation.

#ifndef SRC_SKYPE_VIDEO_SHARED_MEMORY_H_
#define SRC_SKYPE_VIDEO_SHARED_MEMORY_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "build/build_config.h"

#if defined(OS_WIN)
#include "base/win/scoped_handle.h"
#endif

namespace microsoft {

class SharedMemory {
 public:
  static std::unique_ptr<SharedMemory> Open(const std::string& name,
                                            bool read_only);

#if defined(OS_WIN)
  SharedMemory(base::win::ScopedHandle&& section_handle, void* memory);
#else
  SharedMemory(size_t mapped_size, void* memory);
#endif
  ~SharedMemory();

  size_t mapped_size() const { return mapped_size_; }
  void* memory() const { return memory_; }

 private:
#if defined(OS_WIN)
  base::win::ScopedHandle section_handle_;
#endif

  size_t mapped_size_ = 0;
  void* memory_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(SharedMemory);
};

}  // namespace microsoft

#endif  // SRC_SKYPE_VIDEO_SHARED_MEMORY_H_
