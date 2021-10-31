// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "microsoft/src/components/edge_tenant/browser/trv2_plugin_win.h"

#include <memory>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/threading/thread_restrictions.h"
#include "microsoft/src/base/manual_delay_load.h"

class PluginPinTraits : public base::PinSystemLibraryLoader<PluginPinTraits> {
 public:
  constexpr static const base::FilePath::CharType* kName() {
    return FILE_PATH_LITERAL("TenantRestrictionsPlugin.dll");
  }
};
PIN_SYSTEM_DLL(PluginPinTraits)

namespace edge {
namespace tenant_restrictions {
namespace trv2 {
namespace win {

using ScopedAllowLoad = base::ScopedAllowBlockingForTesting;

bool GetIsPluginSupportedFromOs() {
  base::FilePath file;
  ScopedAllowLoad allow_blocking_for_existence_check;
  if (base::PathService::Get(base::DIR_SYSTEM, &file)) {
    file = file.Append(PluginPinTraits::kName());
    return base::PathExists(file);
  }
  return false;
}

void Plugin::LoadHostnamesFunction() {
  struct GetTenantRestrictionsHostnamesTraits {
    typedef PluginPinTraits DllTraits;
    typedef decltype(
        GetTenantRestrictionsHostnamesFunctionDefinition) Signature;
    constexpr static const char* kName() {
      return "GetTenantRestrictionsHostnames";
    }
  };
  ScopedAllowLoad allow_blocking_for_dll_load;
  function_ptr_ =
      base::DelayLoadOptional<GetTenantRestrictionsHostnamesTraits>();
}

Plugin* Plugin::GetPlugin(Plugin* new_plugin) {
  static std::unique_ptr<Plugin> plugin =
      std::make_unique<Plugin>(GetIsPluginSupportedFromOs());
  if (new_plugin) {
    plugin.reset(new_plugin);
  }
  return plugin.get();
}

Plugin::~Plugin() = default;

HRESULT Plugin::CallGetTenantRestrictionsHostname(
    LPWSTR** hostnames,
    UINT32* hostname_count,
    LPWSTR** subdomain_supported_hostnames,
    UINT32* subdomain_supported_hostname_count) {
  CHECK(supported_);
  if (!function_ptr_) {
    LoadHostnamesFunction();
  }
  if (function_ptr_) {
    return function_ptr_(hostnames, hostname_count,
                         subdomain_supported_hostnames,
                         subdomain_supported_hostname_count);
  }
  return E_NOTIMPL;
}
}  // namespace win
}  // namespace trv2
}  // namespace tenant_restrictions
}  // namespace edge
