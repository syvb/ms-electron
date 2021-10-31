// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "microsoft/src/components/edge_tenant/browser/trv2_policy_manager_win.h"

#include <utility>

#include "base/threading/sequenced_task_runner_handle.h"
#include "microsoft/src/components/edge_tenant/browser/trv2_utilities_win.h"

namespace edge {
namespace tenant_restrictions {
namespace trv2 {

WindowsPolicyManager::WindowsPolicyManager() = default;
WindowsPolicyManager::~WindowsPolicyManager() {
  StopWatching();
}

bool WindowsPolicyManager::IsEnabled() {
  return win::Utilities::GetUtilities()->WindowsSupportsTrv2();
}

bool WindowsPolicyManager::OSProvidesPolicy() {
  if (!IsEnabled()) {
    return false;
  }
  return win::Utilities::GetUtilities()->HasPayload();
}

std::string WindowsPolicyManager::GetPayloadFromOS() {
  if (policy_watcher_) {
    return payload_;
  }
  return win::Utilities::GetUtilities()->GetPayload();
}

PluginListType WindowsPolicyManager::GetHostNodeListFromOs() {
  if (policy_watcher_) {
    return host_list_;
  }
  return win::Utilities::GetUtilities()->GetHostList();
}

bool WindowsPolicyManager::StartWatching(
    base::RepeatingCallback<void(bool)> callback) {
  if (!IsEnabled()) {
    return false;
  }
  policy_watcher_ = PolicyWatcher::CreatePolicyWatcher(
      base::BindRepeating(&WindowsPolicyManager::OnPolicyUpdateCallback,
                          weak_ptr_factory_.GetWeakPtr()),
      base::BindRepeating(&WindowsPolicyManager::StopWatching,
                          weak_ptr_factory_.GetWeakPtr()));
  if (policy_watcher_) {
    update_callback_ = std::move(callback);
  }
  return true;
}

void WindowsPolicyManager::StopWatching() {
  update_callback_.Reset();
  if (policy_watcher_) {
    base::SequencedTaskRunnerHandle::Get()->DeleteSoon(
        FROM_HERE, std::move(policy_watcher_));
  }
}

void WindowsPolicyManager::OnPolicyUpdateCallback(UpdateReason reason,
                                                  std::string payload,
                                                  PluginListType list) {
  payload_ = std::move(payload);
  host_list_ = std::move(list);
  if (update_callback_) {
    update_callback_.Run((reason == UpdateReason::kHostList));
  }
}

}  // namespace trv2
}  // namespace tenant_restrictions
}  // namespace edge
