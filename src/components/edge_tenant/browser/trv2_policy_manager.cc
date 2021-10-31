// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "microsoft/src/components/edge_tenant/browser/trv2_policy_manager.h"

#include "base/callback.h"
#include "microsoft/src/components/edge_tenant/common/trv2_policy.h"

namespace edge {
namespace tenant_restrictions {
namespace trv2 {

PolicyManager::PolicyManager() = default;
PolicyManager::~PolicyManager() = default;
bool PolicyManager::ShouldUpdateNetworkService() {
  return true;
}
bool PolicyManager::OSProvidesPolicy() {
  return false;
}
std::string PolicyManager::GetPayloadFromOS() {
  return std::string();
}
std::vector<Policy::HostNode> PolicyManager::GetHostNodeListFromOs() {
  return std::vector<Policy::HostNode>();
}
bool PolicyManager::StartWatching(base::RepeatingCallback<void(bool)>) {
  return false;
}
void PolicyManager::StopWatching() {}

}  // namespace trv2
}  // namespace tenant_restrictions
}  // namespace edge
