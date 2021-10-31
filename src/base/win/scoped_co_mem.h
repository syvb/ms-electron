// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SRC_BASE_WIN_SCOPED_CO_MEM_H_
#define SRC_BASE_WIN_SCOPED_CO_MEM_H_

#include <objbase.h>

#include "base/check.h"
#include "base/macros.h"

namespace microsoft {
namespace base {
namespace win {

// Simple scoped memory releaser class for COM allocated memory.
// Example:
//   base::win::ScopedCoMem<ITEMIDLIST> file_item;
//   SHGetSomeInfo(&file_item, ...);
//   ...
//   return;  <-- memory released
template <typename T>
class ScopedCoMem {
 public:
  explicit ScopedCoMem(T* ptr) : mem_ptr_(ptr) {}
  ScopedCoMem() : ScopedCoMem(nullptr) {}
  ScopedCoMem(ScopedCoMem&& other) : ScopedCoMem(other.Release()) {}

  ~ScopedCoMem() { Reset(nullptr); }

  T** operator&() {  // NOLINT
    // Assume we're about to receive an out parameter so we free any owned
    // memory first so that's it not overwritten by the out parameter and
    // leaked.
    Reset(nullptr);
    return &mem_ptr_;
  }

  operator T*() { return mem_ptr_; }

  T* operator->() {
    DCHECK(mem_ptr_ != nullptr);
    return mem_ptr_;
  }

  const T* operator->() const {
    DCHECK(mem_ptr_ != nullptr);
    return mem_ptr_;
  }

  void Reset(T* ptr = nullptr) {
    if (mem_ptr_)
      CoTaskMemFree(mem_ptr_);
    mem_ptr_ = ptr;
  }

  T* get() const { return mem_ptr_; }

  T* Release() {
    T* ptr = mem_ptr_;
    mem_ptr_ = nullptr;

    return ptr;
  }

 private:
  T* mem_ptr_;

  DISALLOW_COPY_AND_ASSIGN(ScopedCoMem);
};

}  // namespace win
}  // namespace base
}  // namespace microsoft

#endif  // SRC_BASE_WIN_SCOPED_CO_MEM_H_
