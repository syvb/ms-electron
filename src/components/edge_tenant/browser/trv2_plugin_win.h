// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SRC_COMPONENTS_EDGE_TENANT_BROWSER_TRV2_PLUGIN_WIN_H_
#define SRC_COMPONENTS_EDGE_TENANT_BROWSER_TRV2_PLUGIN_WIN_H_

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN
#else
#include <windows.h>
#endif

#include <string>
#include <vector>

#include "microsoft/src/components/edge_tenant/common/trv2_policy.h"

// this function's signature is copied from windows code.
STDAPI GetTenantRestrictionsHostnamesFunctionDefinition(
    LPWSTR** hostnames,
    UINT32* hostnameCount,
    LPWSTR** subdomainSupportedHostnames,
    UINT32* subdomainSupportedHostnameCount);

namespace edge {
namespace tenant_restrictions {
namespace trv2 {
namespace win {
class Plugin {
 public:
  explicit Plugin(bool supported) : supported_(supported) {}
  bool WindowsSupportsTrv2() const { return supported_; }
  virtual ~Plugin();
  HRESULT CallGetTenantRestrictionsHostname(
      LPWSTR** hostnames,
      UINT32* hostname_count,
      LPWSTR** subdomain_supported_hostnames,
      UINT32* subdomain_supported_hostname_count);
  void LoadHostnamesFunction();
  static Plugin* GetPlugin(Plugin* new_plugin = nullptr);

 protected:
  decltype(GetTenantRestrictionsHostnamesFunctionDefinition)* function_ptr_ =
      nullptr;

 private:
  const bool supported_ = true;
};

}  // namespace win
}  // namespace trv2
}  // namespace tenant_restrictions
}  // namespace edge

#endif  // SRC_COMPONENTS_EDGE_TENANT_BROWSER_TRV2_PLUGIN_WIN_H_
