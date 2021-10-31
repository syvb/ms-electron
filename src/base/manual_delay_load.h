// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SRC_BASE_MANUAL_DELAY_LOAD_H_
#define SRC_BASE_MANUAL_DELAY_LOAD_H_

#include "base/base_export.h"
#include "base/macros.h"
#include "base/scoped_native_library.h"

namespace base {

// To operate on many releases of many operating systems, we need a way to try
// to access shared libraries and functions in shared libraries which may not be
// present on a specific release of a specific operating system. These classes
// support transient access to a function in a library (via
// |NativeLibraryLoader| and |SystemLibraryLoader|) or persistent access to a
// function in a library (via PinNativeLibraryLoader and
// PinSystemLibraryLoader). (The origin of the Pin concept is from the Win32 API
// GetModuleHandleEx with the flag GET_MODULE_HANDLE_EX_FLAG_PIN. A pinned
// library stays loaded until the process is terminated.)

// The Pin*LibraryLoader DllTraits base classes keep a static reference to the
// library and guarantee that the library will not be unloaded until the process
// shuts down (that is, all function pointers will continue to be valid while
// they are accessible) and that the library is only loaded once. The
// *LibraryLoader DllTraits base classes allow the dll to be unloaded. (For
// instance, usage in a myfeature::IsEnabled() situation, where if the feature
// is disabled, the dll does not need to stay in memory). When using the
// Pin*LibraryLoader base classes, use the macro PIN_SYSTEM_DLL or PIN_DLL
// to define the Pin*LibraryLoader<>::Load function for your dll.
// The PIN_SYSTEM_DLL macro takes the Derived class name
// THe PIN_DLL macro takes the Derived class name and the name of the function
// that gets the full path of the dll (GetFullPath(...))
// The definition of GetFullPath is:
// base::FilePath GetFullPath(const base::FilePath::CharType* dll_name);
// If you forget the macro, you'll get a LibraryLoader<DerivedClass>::Load is
// not defined linker error.
// If you define the macro multiple times, you'll get a multiple definition
// error.

// The GetFunctionFromNativeLibrary<FunctionTraits>(ScopedLibrary*) template can
// be used with either Pin*Libraryloader or *LibraryLoader DllTraits

// Template Arguments:
// class DllTraits : public LibraryLoader {
//   constexpr static const char* kName() { return "MyDllName"; }
// }
// struct FunctionTraits {
//   typedef MyDllTraits DllTraits;
//   typedef decltype(MyFunctionName) Signature;
//   constexpr static const char* kName() { return "MyFunctionName"; }
// }

// examples:
// // DllTraits
// class MyDllLoadTraits : public NativeLibraryLoader<MyDllLoadTraits> {
//  public:
//   constexpr static const base::FilePath::CharType* kName() {
//     return FILE_PATH_LITERAL("mydll.dll");
//   }
// };
// // forward declare function since we don't have a header
// bool Foo();
// // FunctionTraits
// struct FooTraits {
//   typedef MyDllLoadTraits DllTraits;
//   typedef decltype(Foo) Signature;
//   constexpr static const char* kName() { return "Foo"; }
// };
// // Use GetFunctionFromNativeLibrary<>(ScopedNativeLibrary*):
// // Unload mydll.dll after use
// bool GetFoo() {
//   base::ScopedNativeLibrary library;
//   auto* my_foo = GetFunctionFromNativeLibrary<FooTraits>(&library);
//   return (my_foo) ? my_foo() : false;
// }

// // include header
// #include <bar_header.h> // Bar definition
// // DllTraits
// class MyDllPinTraits : public PinNativeLibraryLoader<MyDllPinTraits> {
//  public:
//   constexpr static const base::FilePath::CharType* kName() {
//     return FILE_PATH_LITERAL("mydll.dll");
//   }
// };
// base::FilePath GetMyDllPath(const base::FilePath::CharType* name) {
//   base::FilePath module_path;
//   bool filepath_found =
//                      base::PathService::Get(base::DIR_MODULE, &module_path);
//   DCHECK(filepath_found);
//   return module_path.Append(name);
// }
// // FunctionTraits
// struct BarFunction {
//   typedef MyDllPinTraits DllTraits;
// };
// struct BarTraits : public BarFunction {
//   typedef decltype(Bar) Signature;
//   constexpr static const char* kName() {
//     return "Bar";
//   }
// };
// struct BarExTraits : public BarFunction {
//   typedef decltype(BarEx) Signature;
//   constexpr static const char* kName() {
//     return "BarEx";
//   }
// };
// // Define PinNativeLibraryLoader<MyDllPinTraits>::Load
// PIN_DLL(MyDllPinTraits, GetMyDllPath);
// // Use DelayLoad<>()
// bool GetBar() {
//   return base::DelayLoad<BarTraits>()();
// }
// bool GetBarEx() {
//   auto* my_bar_ex = base::DelayLoadOptional<BarExTraits>();
//   return (my_bar_ex) ? my_bar_ex() : false;
// }

template <class DerivedClass>
class PinNativeLibraryLoader {
 public:
  constexpr static bool Pinned() { return true; }
  static NativeLibrary Load();

 private:
  static NativeLibrary Pin(const FilePath path) {
    return LoadNativeLibrary(path, nullptr);
  }
};
// function signature for GetFullPathFunction passed in to PIN_DLL :
// base::FilePath GetFullPathFunction(const base::FilePath::CharType* dll_name);
#define PIN_DLL(DerivedClass, GetFullPathFunction)                         \
  template <>                                                              \
  base::NativeLibrary base::PinNativeLibraryLoader<DerivedClass>::Load() { \
    static base::NativeLibrary library =                                   \
        Pin(GetFullPathFunction(DerivedClass::kName()));                   \
    return library;                                                        \
  }

#if defined(OS_WIN)
template <class DerivedClass>
class PinSystemLibraryLoader {
 public:
  constexpr static bool Pinned() { return true; }
  static NativeLibrary Load();

 private:
  static NativeLibrary Pin(const FilePath::CharType* name) {
    return PinSystemLibrary(name, nullptr);
  }
};
#define PIN_SYSTEM_DLL(DerivedClass)                                       \
  template <>                                                              \
  base::NativeLibrary base::PinSystemLibraryLoader<DerivedClass>::Load() { \
    static base::NativeLibrary library = Pin(DerivedClass::kName());       \
    return library;                                                        \
  }
#endif  // OS_WIN

// Let the library unload after use.
class BASE_EXPORT NativeLibraryLoader {
 public:
  constexpr static bool Pinned() { return false; }
  static NativeLibrary Load(const FilePath::CharType* name);
};
#if defined(OS_WIN)
class BASE_EXPORT SystemLibraryLoader {
 public:
  constexpr static bool Pinned() { return false; }
  static NativeLibrary Load(const FilePath::CharType* name);
};
#endif  // OS_WIN

// static helper for delayload scenarios
// Pinning a library so that it doesn't unload until the process is destroyed.
// Usage is optimized for manual delay-loading of functions.
// The template keeps a static variable for the function, so subsequent calls
// will not recalculate the function address. NOTE: this is a static in a
// header, so it is static per .cc file.

// DelayLoadOptional<>() should be used when it is possible the library or
// function cannot be loaded. when using this template, the caller should error
// check and deal with the function returning nullptr.
template <typename FunctionTraits>
typename FunctionTraits::Signature* DelayLoadOptional() {
  static_assert(FunctionTraits::DllTraits::Pinned(),
                "use base::GetFunctionFromNativeLibrary<>()");
  ScopedNativeLibrary module(FunctionTraits::DllTraits::Load());
  if (!module.is_valid()) {
    return nullptr;
  }
  static typename FunctionTraits::Signature* function =
      reinterpret_cast<typename FunctionTraits::Signature*>(
          module.GetFunctionPointer(FunctionTraits::kName()));
  // the library that the ScopedNativeLibrary variable is referencing is pinned,
  // so explicitly leak the reference to the library.
  (void)module.release();
  return function;
}

// DelayLoad<>() should only be used when failing to load the library / funciton
// is an unexpected error and should crash the program.
template <typename FunctionTraits>
typename FunctionTraits::Signature* DelayLoad() {
  typename FunctionTraits::Signature* function =
      DelayLoadOptional<FunctionTraits>();
  CHECK(function);
  return function;
}

template <typename FunctionTraits>
typename FunctionTraits::Signature* GetFunctionFromNativeLibrary(
    ScopedNativeLibrary* library) {
  static_assert(!FunctionTraits::DllTraits::Pinned(),
                "use DelayLoad<>() or DelayLoadOptional<>() instead");
  if (!library->is_valid()) {
    library->reset(
        FunctionTraits::DllTraits::Load(FunctionTraits::DllTraits::kName()));
  }
  if (!library->is_valid()) {
    return nullptr;
  }
  return reinterpret_cast<typename FunctionTraits::Signature*>(
      library->GetFunctionPointer(FunctionTraits::kName()));
}

}  // namespace base

#endif  // SRC_BASE_MANUAL_DELAY_LOAD_H_
