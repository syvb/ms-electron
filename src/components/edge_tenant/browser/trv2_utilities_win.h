// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SRC_COMPONENTS_EDGE_TENANT_BROWSER_TRV2_UTILITIES_WIN_H_
#define SRC_COMPONENTS_EDGE_TENANT_BROWSER_TRV2_UTILITIES_WIN_H_

#include <string>
#include <vector>

#include "base/win/registry.h"
#include "microsoft/src/components/edge_tenant/common/trv2_policy.h"

namespace edge {
namespace tenant_restrictions {
namespace trv2 {
using PluginListType = std::vector<Policy::HostNode>;
using PrototypeListType = std::vector<std::string>;
namespace win {
class Utilities {
 public:
  Utilities();
  virtual ~Utilities();
  virtual base::win::RegKey GetPolicyRegKey();
  virtual bool HasPayload();
  virtual std::string GetPayload();
  virtual PluginListType GetHostList();
  virtual bool WindowsSupportsTrv2();

  static Utilities* GetUtilities(Utilities* new_utilities = nullptr);
};
}  // namespace win
}  // namespace trv2
}  // namespace tenant_restrictions
}  // namespace edge

#endif  // SRC_COMPONENTS_EDGE_TENANT_BROWSER_TRV2_UTILITIES_WIN_H_
