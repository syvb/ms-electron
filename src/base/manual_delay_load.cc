// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "microsoft/src/base/manual_delay_load.h"

namespace base {

#if defined(OS_WIN)
NativeLibrary SystemLibraryLoader::Load(const FilePath::CharType* name) {
  return LoadSystemLibrary(name, nullptr);
}
#endif  // OS_WIN

NativeLibrary NativeLibraryLoader::Load(const FilePath::CharType* name) {
  return LoadNativeLibrary(FilePath(name), nullptr);
}

}  // namespace base
