// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "microsoft/src/components/edge_tenant/browser/trv2_policy_watcher_win.h"

#include <utility>

#include "base/callback.h"
#include "microsoft/src/components/edge_tenant/browser/trv2_utilities_win.h"
#include "microsoft/src/components/edge_tenant/browser/trv2_watchers_win.h"

namespace edge {
namespace tenant_restrictions {
namespace trv2 {

PolicyWatcher::PolicyWatcher() {
  group_policy_watcher_ =
      std::make_unique<GroupPolicyWatcher>(base::BindRepeating(
          &PolicyWatcher::OnPolicyUpdate, weak_ptr_factory_.GetWeakPtr()));
  host_list_watcher_ = std::make_unique<HostListWatcher>(base::BindRepeating(
      &PolicyWatcher::OnHostListUpdate, weak_ptr_factory_.GetWeakPtr()));
}

std::unique_ptr<PolicyWatcher> PolicyWatcher::CreatePolicyWatcher(
    UpdateCallback update_callback,
    base::RepeatingCallback<void()> watcher_failed_callback) {
  std::unique_ptr<PolicyWatcher> watcher = std::make_unique<PolicyWatcher>();
  bool success = watcher->Initialize(std::move(update_callback),
                                     std::move(watcher_failed_callback));
  if (success) {
    return watcher;
  }
  return nullptr;
}

bool PolicyWatcher::Initialize(
    UpdateCallback update_callback,
    base::RepeatingCallback<void()> watcher_failed_callback) {
  StartWatchingGP();
  if (watcher_failed_callback_run_) {
    // windows failed us. we're not watching...
    return false;
  }
  StartWatchingHostList();
  if (watcher_failed_callback_run_) {
    return false;
  }

  update_callback_ = std::move(update_callback);
  watcher_failed_callback_ = std::move(watcher_failed_callback);
  initialized_ = true;

  RunUpdate(UpdateReason::kPolicy);
  return true;
}

PolicyWatcher::~PolicyWatcher() {
  CleanUpWatchers();
}

void PolicyWatcher::CleanUpWatchers() {
  group_policy_watcher_.reset();
  host_list_watcher_.reset();
}

void PolicyWatcher::RunUpdate(UpdateReason reason) {
  DCHECK(update_callback_.MaybeValid());
  if (win::Utilities::GetUtilities()->HasPayload()) {
    update_callback_.Run(reason, win::Utilities::GetUtilities()->GetPayload(),
                         win::Utilities::GetUtilities()->GetHostList());
  } else {
    update_callback_.Run(reason, std::string(), PluginListType());
  }
}

void PolicyWatcher::RunWatcherFailedCallbackIfNeeded(bool condition) {
  if (!condition) {
    watcher_failed_callback_run_ = true;
    if (group_policy_watcher_->IsInitialized()) {
      group_policy_watcher_->StopWatching();
    }
    if (host_list_watcher_->IsInitialized()) {
      host_list_watcher_->StopWatching();
    }
    if (initialized_) {
      watcher_failed_callback_.Run();
    }
  }
}

void PolicyWatcher::OnPolicyUpdate() {
  if (watcher_failed_callback_run_) {
    // something failed earlier; bail so cleanup can happen.
    return;
  }
  StartWatchingGP();
  if (watcher_failed_callback_run_) {
    // don't do an update, watch failed callback was run
    return;
  }
  return RunUpdate(UpdateReason::kPolicy);
}

void PolicyWatcher::StartWatchingGP() {
  if (watcher_failed_callback_run_) {
    // something failed earlier; bail so cleanup can happen.
    return;
  }
  RunWatcherFailedCallbackIfNeeded(group_policy_watcher_->StartWatching());
}

void PolicyWatcher::StopWatchingGP() {
  group_policy_watcher_->StopWatching();
}

void PolicyWatcher::OnHostListUpdate() {
  if (watcher_failed_callback_run_) {
    // something failed earlier; bail so cleanup can happen.
    return;
  }
  StartWatchingHostList();
  if (watcher_failed_callback_run_) {
    // don't do an update, watch failed callback was run
    return;
  }
  RunUpdate(UpdateReason::kHostList);
}

void PolicyWatcher::StartWatchingHostList() {
  if (watcher_failed_callback_run_) {
    // something failed earlier; bail so cleanup can happen.
    return;
  }
  RunWatcherFailedCallbackIfNeeded(host_list_watcher_->StartWatching());
}

void PolicyWatcher::StopWatchingHostList() {
  host_list_watcher_->StopWatching();
}

}  // namespace trv2
}  // namespace tenant_restrictions
}  // namespace edge
