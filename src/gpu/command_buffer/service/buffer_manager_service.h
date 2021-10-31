// Copyright (c) 2019 Microsoft Corporation.

#ifndef SRC_GPU_COMMAND_BUFFER_SERVICE_BUFFER_MANAGER_SERVICE_H_
#define SRC_GPU_COMMAND_BUFFER_SERVICE_BUFFER_MANAGER_SERVICE_H_

#include <map>
#include <memory>
#include <string>

namespace microsoft {

class SharedMemory;

class BufferManagerService {
 public:
  BufferManagerService();
  ~BufferManagerService();

  std::shared_ptr<SharedMemory> GetBuffer(const std::string& name);
  void ReleaseBuffer(const std::string& name);

 private:
  std::map<std::string, std::shared_ptr<SharedMemory>> buffer_map_;
};

}  // namespace microsoft

#endif  // SRC_GPU_COMMAND_BUFFER_SERVICE_BUFFER_MANAGER_SERVICE_H_
