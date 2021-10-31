// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SRC_COMPONENTS_EDGE_TENANT_BROWSER_TRV2_WATCHERS_WIN_H_
#define SRC_COMPONENTS_EDGE_TENANT_BROWSER_TRV2_WATCHERS_WIN_H_

#include "base/callback.h"
#include "base/memory/weak_ptr.h"

#include "base/synchronization/waitable_event.h"
#include "base/synchronization/waitable_event_watcher.h"
#include "base/win/registry.h"

namespace edge {
namespace tenant_restrictions {
namespace trv2 {
class Watcher {
 public:
  explicit Watcher(base::RepeatingCallback<void()> callback);
  virtual ~Watcher();
  virtual bool StartWatching() = 0;
  virtual void StopWatching() {}
  virtual bool IsInitialized() = 0;
  bool IsWatching() { return is_watching_; }
  void OnUpdate() {
    is_watching_ = false;
    callback_.Run();
  }

 protected:
  bool is_watching_ = false;

 private:
  base::RepeatingCallback<void()> callback_;
};

class GroupPolicyWatcher final : public Watcher {
 public:
  explicit GroupPolicyWatcher(base::RepeatingCallback<void()> callback);
  ~GroupPolicyWatcher() override;
  bool StartWatching() override;
  void StopWatching() override;
  bool IsInitialized() override;
  void OnPolicyUpdate(base::WaitableEvent*);

 private:
  bool is_registered_for_notifications_ = false;
  base::WaitableEvent machine_policy_changed_event_{
      base::WaitableEvent::ResetPolicy::AUTOMATIC,
      base::WaitableEvent::InitialState::NOT_SIGNALED};
  base::WaitableEventWatcher machine_policy_watcher_;

  base::WeakPtrFactory<GroupPolicyWatcher> weak_ptr_factory_{this};
};

class HostListWatcher final : public Watcher {
 public:
  explicit HostListWatcher(base::RepeatingCallback<void()> callback);
  ~HostListWatcher() override;
  bool StartWatching() override;
  void StopWatching() override;
  bool IsInitialized() override;

 private:
  bool inititialized_ = false;
  base::WeakPtrFactory<HostListWatcher> weak_ptr_factory_{this};
};

}  // namespace trv2
}  // namespace tenant_restrictions
}  // namespace edge

#endif  // SRC_COMPONENTS_EDGE_TENANT_BROWSER_TRV2_WATCHERS_WIN_H_
