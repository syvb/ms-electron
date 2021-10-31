// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SRC_COMPONENTS_EDGE_TENANT_BROWSER_TRV2_POLICY_MANAGER_WIN_H_
#define SRC_COMPONENTS_EDGE_TENANT_BROWSER_TRV2_POLICY_MANAGER_WIN_H_

#include "microsoft/src/components/edge_tenant/browser/trv2_policy_manager.h"

#include <memory>
#include <string>

#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "microsoft/src/components/edge_tenant/browser/trv2_policy_watcher_win.h"
#include "microsoft/src/components/edge_tenant/common/trv2_policy.h"

namespace edge {
namespace tenant_restrictions {
namespace trv2 {

class WindowsPolicyManager final : public PolicyManager {
 public:
  WindowsPolicyManager();
  ~WindowsPolicyManager() override;

  bool OSProvidesPolicy() override;
  std::string GetPayloadFromOS() override;
  PluginListType GetHostNodeListFromOs() override;
  bool StartWatching(base::RepeatingCallback<void(bool)> callback) override;
  void StopWatching() override;

  bool IsEnabled();

 private:
  base::RepeatingCallback<void(bool)> update_callback_;

  void OnPolicyUpdateCallback(UpdateReason reason,
                              std::string payload,
                              PluginListType list);

  std::string payload_;
  PluginListType host_list_;

  // policy watcher
  std::unique_ptr<PolicyWatcher> policy_watcher_;

  base::WeakPtrFactory<WindowsPolicyManager> weak_ptr_factory_{this};
};

}  // namespace trv2
}  // namespace tenant_restrictions
}  // namespace edge

#endif  // SRC_COMPONENTS_EDGE_TENANT_BROWSER_TRV2_POLICY_MANAGER_WIN_H_
