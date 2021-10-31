// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "microsoft/src/components/edge_tenant/browser/trv2_update_manager.h"

#include <string>
#include <utility>
#include <vector>

#include "base/callback.h"
#include "content/public/browser/network_service_instance.h"
#include "microsoft/src/components/edge_tenant/browser/trv2_policy_manager.h"
#include "microsoft/src/components/edge_tenant/common/trv2_policy.h"
#include "services/network/public/mojom/network_service.mojom.h"

#ifdef OS_WIN
#include "microsoft/src/components/edge_tenant/browser/trv2_policy_manager_win.h"
#endif

namespace edge {
namespace tenant_restrictions {
namespace trv2 {
std::unique_ptr<UpdateManager> UpdateManager::CreateUpdateManager() {
  std::unique_ptr<UpdateManager> manager = std::make_unique<UpdateManager>();
  if (manager->InitPolicyManager()) {
    return manager;
  }
  return nullptr;
}
std::unique_ptr<UpdateManager> UpdateManager::CreateUpdateManagerForTesting(
    PolicyManager* policy_manager) {
  std::unique_ptr<UpdateManager> manager = std::make_unique<UpdateManager>();
  manager->SetPolicyManagerForTesting(policy_manager);
  if (manager->InitPolicyManager()) {
    return manager;
  }
  return nullptr;
}

UpdateManager::UpdateManager() = default;
UpdateManager::~UpdateManager() = default;

bool UpdateManager::InitPolicyManager() {
  if (!manager_) {
#ifdef OS_WIN
    manager_ = std::make_unique<WindowsPolicyManager>();
#else
    manager_ = std::make_unique<PolicyManager>();
#endif
  }
  return manager_->StartWatching(
      base::BindRepeating(&UpdateManager::OnUpdateFromPolicyManager,
                          weak_ptr_factory_.GetWeakPtr()));
}

void UpdateManager::OnUpdateFromPolicyManager(bool force_update) {
  if (force_update) {
    ForceUpdate();
  } else {
    UpdateIfNeeded();
  }
}

void UpdateManager::RunUpdate(base::Optional<std::string> new_payload) {
  std::unique_ptr<Policy> policy;
  if (manager_->OSProvidesPolicy()) {
    std::string payload =
        (new_payload) ? *new_payload : manager_->GetPayloadFromOS();
    std::vector<Policy::HostNode> hosts = manager_->GetHostNodeListFromOs();
    policy = std::make_unique<Policy>(payload, hosts);
  }
  if (!policy || policy->IsValid()) {
    OnPolicyUpdate(std::move(policy));
  }
}

void UpdateManager::ForceUpdate() {
  RunUpdate(base::nullopt);
}

bool UpdateManager::UpdateIfNeeded() {
  bool is_on = !!current_policies_;
  bool os_has_policies = manager_->OSProvidesPolicy();
  // update if trv2 has turned on or off
  if (is_on != os_has_policies) {
    if (!os_has_policies) {
      OnPolicyUpdate(nullptr);
    } else {
      RunUpdate(base::nullopt);
    }
    return true;
  }
  // update if the payload has changed
  if (is_on) {
    std::string new_payload = manager_->GetPayloadFromOS();
    if (current_policies_->payload() != new_payload) {
      RunUpdate(std::move(new_payload));
      return true;
    }
  }
  return false;
}

void UpdateManager::OnPolicyUpdate(std::unique_ptr<Policy> policy) {
  current_policies_ =
      ((policy) && policy->IsActive()) ? std::move(policy) : nullptr;
  if (!manager_->ShouldUpdateNetworkService()) {
    return;
  }
  if (current_policies_) {
    content::GetNetworkService()->UpdateTrv2HeaderPolicies(
        current_policies_->payload(), current_policies_->PackHosts());
  } else {
    content::GetNetworkService()->UpdateTrv2HeaderPolicies(
        std::string(), std::vector<std::string>());
  }
}

void UpdateManager::SetPolicyManagerForTesting(PolicyManager* manager) {
  manager_.reset(manager);
}

}  // namespace trv2
}  // namespace tenant_restrictions
}  // namespace edge
