// Copyright (c) 2019 Microsoft Corporation.

#include "microsoft/src/gpu/command_buffer/service/buffer_manager_service.h"

#include "microsoft/src/skype_video/shared_memory.h"

namespace microsoft {

// Complex class/struct needs an explicit out-of-line constructor.
BufferManagerService::BufferManagerService() = default;

// Complex class/struct needs an explicit out-of-line destructor.
BufferManagerService::~BufferManagerService() = default;

std::shared_ptr<SharedMemory> BufferManagerService::GetBuffer(
    const std::string& name) {
  auto it = buffer_map_.find(name);
  if (it != buffer_map_.end()) {
    return it->second;
  }

  auto shm = std::shared_ptr<SharedMemory>(SharedMemory::Open(name, true));
  if (!shm || !shm->memory()) {
    return nullptr;
  }

  buffer_map_.emplace(name, shm);

  return shm;
}

void BufferManagerService::ReleaseBuffer(const std::string& name) {
  buffer_map_.erase(name);
}

}  // namespace microsoft
