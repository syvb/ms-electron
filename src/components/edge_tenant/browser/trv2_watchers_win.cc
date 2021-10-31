// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "microsoft/src/components/edge_tenant/browser/trv2_watchers_win.h"

#include <userenv.h>  // for RegisterGPNotification

#include <utility>

#include "base/callback.h"
#include "base/logging.h"
#include "base/synchronization/waitable_event.h"
#include "base/synchronization/waitable_event_watcher.h"
#include "base/win/registry.h"
#include "microsoft/src/components/edge_tenant/browser/trv2_utilities_win.h"

namespace edge {
namespace tenant_restrictions {
namespace trv2 {

Watcher::Watcher(base::RepeatingCallback<void()> callback)
    : callback_(callback) {}
Watcher::~Watcher() {
  StopWatching();
}

GroupPolicyWatcher::GroupPolicyWatcher(base::RepeatingCallback<void()> callback)
    : Watcher(std::move(callback)) {}
GroupPolicyWatcher::~GroupPolicyWatcher() = default;
bool GroupPolicyWatcher::StartWatching() {
  if (!is_registered_for_notifications_) {
    is_registered_for_notifications_ =
        ::RegisterGPNotification(machine_policy_changed_event_.handle(), true);
  }

  if (!is_registered_for_notifications_) {
    DPLOG(ERROR) << __FUNCTION__ << "Registering for gp notifications failed. ";
  }

  if (is_registered_for_notifications_ && !is_watching_) {
    is_watching_ = machine_policy_watcher_.StartWatching(
        &machine_policy_changed_event_,
        base::BindOnce(&GroupPolicyWatcher::OnPolicyUpdate,
                       weak_ptr_factory_.GetWeakPtr()),
        nullptr);
  }
  return is_watching_;
}
void GroupPolicyWatcher::StopWatching() {
  if (is_registered_for_notifications_) {
    ::UnregisterGPNotification(machine_policy_changed_event_.handle());
    is_registered_for_notifications_ = false;
  }
  if (is_watching_) {
    machine_policy_watcher_.StopWatching();
    is_watching_ = false;
  }
}
bool GroupPolicyWatcher::IsInitialized() {
  return is_registered_for_notifications_;
}

void GroupPolicyWatcher::OnPolicyUpdate(base::WaitableEvent*) {
  OnUpdate();
}

HostListWatcher::HostListWatcher(base::RepeatingCallback<void()> callback)
    : Watcher(std::move(callback)) {}
HostListWatcher::~HostListWatcher() = default;
bool HostListWatcher::StartWatching() {
  is_watching_ = true;
  inititialized_ = true;
  return is_watching_;
}
void HostListWatcher::StopWatching() {
  is_watching_ = false;
}
bool HostListWatcher::IsInitialized() {
  return inititialized_;
}

}  // namespace trv2
}  // namespace tenant_restrictions
}  // namespace edge
