// Copyright (c) 2019 Microsoft Corporation.

#include "microsoft/src/sandbox/win/section_interception.h"

#include <cstdint>
#include <memory>

#include "sandbox/win/src/crosscall_client.h"
#include "sandbox/win/src/ipc_tags.h"
#include "sandbox/win/src/policy_params.h"
#include "sandbox/win/src/policy_target.h"
#include "sandbox/win/src/sandbox_factory.h"
#include "sandbox/win/src/sandbox_nt_util.h"
#include "sandbox/win/src/sharedmem_ipc_client.h"
#include "sandbox/win/src/target_services.h"

namespace sandbox {

ResultCode ProxyOpenSection(LPCWSTR name,
                            uint32_t desired_access,
                            void* ipc_memory,
                            CrossCallReturn* answer) {
  CountedParameterSet<OpenSectionParams> params;
  params[OpenSectionParams::NAME] = ParamPickerMake(name);
  params[OpenSectionParams::ACCESS] = ParamPickerMake(desired_access);

  if (!QueryBroker(IpcTag::NTOPENSECTION, params.GetBase()))
    return SBOX_ERROR_GENERIC;

  SharedMemIPCClient ipc(ipc_memory);
  ResultCode code =
      CrossCall(ipc, IpcTag::NTOPENSECTION, name, desired_access, answer);

  return code;
}

NTSTATUS WINAPI TargetNtOpenSection(NtOpenSectionFunction orig_OpenSection,
                                    PHANDLE section_handle,
                                    ACCESS_MASK desired_access,
                                    POBJECT_ATTRIBUTES object_attributes) {
  NTSTATUS status =
      orig_OpenSection(section_handle, desired_access, object_attributes);
  if (status != STATUS_ACCESS_DENIED || !object_attributes)
    return status;

  // We don't trust that the IPC can work this early.
  if (!SandboxFactory::GetTargetServices()->GetState()->InitCalled())
    return status;

  do {
    if (!ValidParameter(section_handle, sizeof(HANDLE), WRITE))
      break;

    void* memory = GetGlobalIPCMemory();
    if (!memory)
      break;

    OBJECT_ATTRIBUTES object_attribs_copy = *object_attributes;
    // The RootDirectory points to BaseNamedObjects. We can ignore it.
    object_attribs_copy.RootDirectory = nullptr;

    std::unique_ptr<wchar_t, NtAllocDeleter> name;
    uint32_t attributes = 0;
    NTSTATUS ret =
        AllocAndCopyName(&object_attribs_copy, &name, &attributes, nullptr);
    if (!NT_SUCCESS(ret) || !name)
      break;

    CrossCallReturn answer = {0};
    answer.nt_status = status;
    ResultCode code =
        ProxyOpenSection(name.get(), desired_access, memory, &answer);

    if (code != SBOX_ALL_OK) {
      status = answer.nt_status;
      break;
    }
    __try {
      *section_handle = answer.handle;
      status = STATUS_SUCCESS;
    } __except (EXCEPTION_EXECUTE_HANDLER) {  // NOLINT
      break;
    }
  } while (false);

  return status;
}

}  // namespace sandbox
