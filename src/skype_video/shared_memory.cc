// Copyright (c) 2019 Microsoft Corporation.

#include "microsoft/src/skype_video/shared_memory.h"

#include <utility>

#include "base/logging.h"
#include "base/strings/utf_string_conversions.h"

#if defined(OS_WIN)
#include <windows.h>
#endif

#if defined(OS_POSIX)
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "base/files/scoped_file.h"
#endif

// This implementation must be able to open shared memory created by
// https://skype.visualstudio.com/SCC/_git/client-shared_electron_slimcore?path=%2Fsrc%2Futils%2FSharedMemory.h&version=GBmaster

namespace microsoft {

#if defined(OS_WIN)

// Returns the length of the memory section starting at the supplied address.
static size_t GetMemorySectionSize(void* address) {
  MEMORY_BASIC_INFORMATION memory_info;
  if (!VirtualQuery(address, &memory_info, sizeof(memory_info))) {
    LOG(ERROR) << "GetMemorySectionSize - VirtualQuery failed";
    return 0;
  }

  return memory_info.RegionSize -
         (static_cast<char*>(address) -
          static_cast<char*>(memory_info.AllocationBase));
}

std::unique_ptr<SharedMemory> SharedMemory::Open(const std::string& name,
                                                 bool read_only) {
  auto section_name =
      base::UTF8ToWide("Local\\VideoRenderer::SharedMemory::" + name);

  DWORD access = read_only ? FILE_MAP_READ : FILE_MAP_WRITE;

  base::win::ScopedHandle section_handle(
      OpenFileMapping(access, false, section_name.c_str()));

  if (!section_handle.IsValid()) {
    LOG(ERROR) << "SharedMemory::Open - OpenFileMapping failed";
    return nullptr;
  }

  auto* memory = MapViewOfFile(section_handle.Get(), access, 0, 0, 0);

  if (!memory) {
    LOG(ERROR) << "SharedMemory::Open - MapViewOfFile failed";
    return nullptr;
  }

  return std::make_unique<SharedMemory>(std::move(section_handle), memory);
}

SharedMemory::SharedMemory(base::win::ScopedHandle&& section_handle,
                           void* memory)
    : section_handle_(std::move(section_handle)),
      mapped_size_(GetMemorySectionSize(memory)),
      memory_(memory) {
  DCHECK(section_handle_.IsValid());
  DCHECK(mapped_size_);
  DCHECK(memory_);
}

SharedMemory::~SharedMemory() {
  UnmapViewOfFile(memory_);
}

#elif defined(OS_POSIX)

std::unique_ptr<SharedMemory> SharedMemory::Open(const std::string& name,
                                                 bool read_only) {
  std::string filename = "/shm-" + name;
  base::ScopedFD fd(
      shm_open(filename.c_str(), read_only ? O_RDONLY : O_RDWR, 0666));

  if (!fd.is_valid()) {
    filename = "msvdrndr-" + name;
    fd = base::ScopedFD(
        shm_open(filename.c_str(), read_only ? O_RDONLY : O_RDWR, 0666));
  }

  if (!fd.is_valid()) {
    LOG(ERROR) << "SharedMemory::Open - shm_open failed";
    return nullptr;
  }

  struct stat st;
  if (fstat(fd.get(), &st) != 0 || st.st_size < 0) {
    LOG(ERROR) << "SharedMemory::Open - fstat failed";
    return nullptr;
  }

  auto mapped_size = st.st_size;

  int prot = PROT_READ | (read_only ? 0 : PROT_WRITE);
  auto* memory = mmap(nullptr, mapped_size, prot, MAP_SHARED, fd.get(), 0);

  if (memory == MAP_FAILED) {
    LOG(ERROR) << "SharedMemory::Open - mmap failed";
    return nullptr;
  }

  return std::make_unique<SharedMemory>(mapped_size, memory);
}

SharedMemory::SharedMemory(size_t mapped_size, void* memory)
    : mapped_size_(mapped_size), memory_(memory) {
  DCHECK(mapped_size_);
  DCHECK(memory_);
}

SharedMemory::~SharedMemory() {
  munmap(memory_, mapped_size_);
}

#else
#error Unsupported platform
#endif

}  // namespace microsoft
