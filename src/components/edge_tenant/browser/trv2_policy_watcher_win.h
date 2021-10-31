// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SRC_COMPONENTS_EDGE_TENANT_BROWSER_TRV2_POLICY_WATCHER_WIN_H_
#define SRC_COMPONENTS_EDGE_TENANT_BROWSER_TRV2_POLICY_WATCHER_WIN_H_

#include "microsoft/src/components/edge_tenant/browser/trv2_policy_manager.h"

#include <memory>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "microsoft/src/components/edge_tenant/browser/trv2_watchers_win.h"
#include "microsoft/src/components/edge_tenant/common/trv2_policy.h"

namespace edge {
namespace tenant_restrictions {
namespace trv2 {
enum class UpdateReason { kPolicy, kHostList };
using PluginListType = std::vector<Policy::HostNode>;
using UpdateCallback =
    base::RepeatingCallback<void(UpdateReason, std::string, PluginListType)>;

class PolicyWatcher {
 public:
  static std::unique_ptr<PolicyWatcher> CreatePolicyWatcher(
      UpdateCallback update_callback,
      base::RepeatingCallback<void()> watcher_failed_callback);

  PolicyWatcher();
  ~PolicyWatcher();
  bool Initialize(UpdateCallback update_callback,
                  base::RepeatingCallback<void()> watcher_failed_callback);

 private:
  friend class Trv2PolicyWatcherTest;
  friend class Trv2PolicyWatcherPrototypeTest;

  void CleanUpWatchers();

  void StartWatchingGP();
  void StopWatchingGP();
  void OnPolicyUpdate();

  void StartWatchingHostList();
  void StopWatchingHostList();
  void OnHostListUpdate();

  void RunUpdate(UpdateReason reason);
  void RunWatcherFailedCallbackIfNeeded(bool condition);

  std::unique_ptr<Watcher> group_policy_watcher_;
  std::unique_ptr<Watcher> host_list_watcher_;

  UpdateCallback update_callback_;
  base::RepeatingCallback<void()> watcher_failed_callback_;
  bool watcher_failed_callback_run_ = false;

  bool initialized_ = false;

  base::WeakPtrFactory<PolicyWatcher> weak_ptr_factory_{this};
};
}  // namespace trv2
}  // namespace tenant_restrictions
}  // namespace edge

#endif  // SRC_COMPONENTS_EDGE_TENANT_BROWSER_TRV2_POLICY_WATCHER_WIN_H_
