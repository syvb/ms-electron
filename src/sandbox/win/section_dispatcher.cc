// Copyright (c) 2019 Microsoft Corporation.

#include "microsoft/src/sandbox/win/section_dispatcher.h"

#include <cstdint>

#include "base/win/windows_version.h"
#include "sandbox/win/src/crosscall_client.h"
#include "sandbox/win/src/interception.h"
#include "sandbox/win/src/interceptors.h"
#include "sandbox/win/src/ipc_tags.h"
#include "sandbox/win/src/policy_broker.h"
#include "sandbox/win/src/policy_params.h"
#include "sandbox/win/src/sandbox.h"

#include "microsoft/src/sandbox/win/section_interception.h"
#include "microsoft/src/sandbox/win/section_policy.h"

namespace sandbox {

SectionDispatcher::SectionDispatcher(PolicyBase* policy_base)
    : policy_base_(policy_base) {
  static const IPCCall open_params = {
      {IpcTag::NTOPENSECTION, {WCHAR_TYPE, UINT32_TYPE}},
      reinterpret_cast<CallbackGeneric>(&SectionDispatcher::NtOpenSection)};

  ipc_calls_.push_back(open_params);
}

bool SectionDispatcher::SetupService(InterceptionManager* manager,
                                     IpcTag service) {
  return (service == IpcTag::NTOPENSECTION) &&
         INTERCEPT_NT(manager, NtOpenSection, OPEN_SECTION_ID, 16);
}

bool SectionDispatcher::NtOpenSection(IPCInfo* ipc,
                                      base::string16* name,
                                      uint32_t desired_access) {
  const wchar_t* section_name = name->c_str();

  CountedParameterSet<OpenSectionParams> params;
  params[OpenSectionParams::NAME] = ParamPickerMake(section_name);
  params[OpenSectionParams::ACCESS] = ParamPickerMake(desired_access);

  EvalResult result =
      policy_base_->EvalPolicy(IpcTag::NTOPENSECTION, params.GetBase());
  HANDLE handle = nullptr;
  // Return operation status on the IPC.
  ipc->return_info.nt_status = SectionPolicy::OpenSectionAction(
      result, *ipc->client_info, *name, desired_access, &handle);
  ipc->return_info.handle = handle;
  return true;
}

}  // namespace sandbox
