// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SRC_COMPONENTS_EDGE_TENANT_BROWSER_TRV2_POLICY_MANAGER_H_
#define SRC_COMPONENTS_EDGE_TENANT_BROWSER_TRV2_POLICY_MANAGER_H_

#include <string>
#include <vector>

#include "base/callback_forward.h"
#include "microsoft/src/components/edge_tenant/common/trv2_policy.h"

namespace edge {
namespace tenant_restrictions {
namespace trv2 {

class PolicyManager {
 public:
  PolicyManager();
  virtual ~PolicyManager();
  virtual bool ShouldUpdateNetworkService();
  virtual bool OSProvidesPolicy();
  virtual std::string GetPayloadFromOS();
  virtual std::vector<Policy::HostNode> GetHostNodeListFromOs();
  virtual bool StartWatching(base::RepeatingCallback<void(bool)> callback);
  virtual void StopWatching();
};

}  // namespace trv2
}  // namespace tenant_restrictions
}  // namespace edge

#endif  // SRC_COMPONENTS_EDGE_TENANT_BROWSER_TRV2_POLICY_MANAGER_H_
