// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SRC_COMPONENTS_EDGE_TENANT_BROWSER_TRV2_UPDATE_MANAGER_H_
#define SRC_COMPONENTS_EDGE_TENANT_BROWSER_TRV2_UPDATE_MANAGER_H_

#include <memory>
#include <string>

#include "base/callback_forward.h"
#include "base/memory/weak_ptr.h"
#include "base/optional.h"

namespace edge {
namespace tenant_restrictions {
namespace trv2 {

class Policy;
class PolicyManager;

class UpdateManager {
 public:
  UpdateManager();
  ~UpdateManager();
  static std::unique_ptr<UpdateManager> CreateUpdateManager();
  static std::unique_ptr<UpdateManager> CreateUpdateManagerForTesting(
      PolicyManager* policy_manager);
  void ForceUpdate();
  bool UpdateIfNeeded();

 private:
  friend class Trv2UpdateManagerTest;
  friend class Trv2UpdateManagerPrototypeTest;

  // For testing
  void SetPolicyManagerForTesting(PolicyManager* manager);

  void RunUpdate(base::Optional<std::string> new_payload);
  void OnPolicyUpdate(std::unique_ptr<Policy> policy);
  bool InitPolicyManager();
  void OnUpdateFromPolicyManager(bool force_update);
  std::unique_ptr<PolicyManager> manager_;
  std::unique_ptr<Policy> current_policies_;

  base::WeakPtrFactory<UpdateManager> weak_ptr_factory_{this};
};

}  // namespace trv2
}  // namespace tenant_restrictions
}  // namespace edge

#endif  // SRC_COMPONENTS_EDGE_TENANT_BROWSER_TRV2_UPDATE_MANAGER_H_
