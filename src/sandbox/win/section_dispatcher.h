// Copyright (c) 2019 Microsoft Corporation.

#ifndef SRC_SANDBOX_WIN_SECTION_DISPATCHER_H_
#define SRC_SANDBOX_WIN_SECTION_DISPATCHER_H_

#include <cstdint>

#include "base/macros.h"
#include "base/strings/string16.h"
#include "sandbox/win/src/crosscall_server.h"
#include "sandbox/win/src/ipc_tags.h"
#include "sandbox/win/src/sandbox_policy_base.h"

namespace sandbox {

// This class handles section-related IPC calls.
class SectionDispatcher : public Dispatcher {
 public:
  explicit SectionDispatcher(PolicyBase* policy_base);
  ~SectionDispatcher() override {}

  // Dispatcher interface.
  bool SetupService(InterceptionManager* manager, IpcTag service) override;

 private:
  // Processes IPC requests coming from calls to NtOpenSection in the target.
  bool NtOpenSection(IPCInfo* ipc,
                     base::string16* name,
                     uint32_t desired_access);

  PolicyBase* policy_base_;
  DISALLOW_COPY_AND_ASSIGN(SectionDispatcher);
};

}  // namespace sandbox

#endif  // SRC_SANDBOX_WIN_SECTION_DISPATCHER_H_
