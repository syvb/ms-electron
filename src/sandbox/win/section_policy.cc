// Copyright (c) 2019 Microsoft Corporation.

#include "microsoft/src/sandbox/win/section_policy.h"

#include <cstdint>
#include <string>

#include "base/logging.h"
#include "base/strings/stringprintf.h"
#include "sandbox/win/src/ipc_tags.h"
#include "sandbox/win/src/nt_internals.h"
#include "sandbox/win/src/policy_engine_opcodes.h"
#include "sandbox/win/src/policy_params.h"
#include "sandbox/win/src/sandbox_types.h"
#include "sandbox/win/src/sandbox_utils.h"
#include "sandbox/win/src/win_utils.h"

#include "microsoft/src/sandbox/win/section_interception.h"

namespace sandbox {

NTSTATUS GetBaseNamedObjectsDirectory(HANDLE* directory);

bool SectionPolicy::GenerateRules(const wchar_t* name,
                                  TargetPolicy::Semantics semantics,
                                  LowLevelPolicy* policy) {
  base::string16 mod_name(name);
  if (mod_name.empty()) {
    return false;
  }

  if (TargetPolicy::SECTIONS_ALLOW_ANY != semantics &&
      TargetPolicy::SECTIONS_ALLOW_READONLY != semantics) {
    // Other flags are not valid for sections policy yet.
    NOTREACHED();
    return false;
  }

  // Add the open rule.
  EvalResult result = ASK_BROKER;
  PolicyRule open(result);

  if (!open.AddStringMatch(IF, OpenSectionParams::NAME, name, CASE_INSENSITIVE))
    return false;

  if (TargetPolicy::SECTIONS_ALLOW_READONLY == semantics) {
    // We consider all flags that are not known to be readonly as potentially
    // used for write.
    uint32_t allowed_flags = SECTION_QUERY | SECTION_MAP_READ;
    uint32_t restricted_flags = ~allowed_flags;
    open.AddNumberMatch(IF_NOT, OpenSectionParams::ACCESS, restricted_flags,
                        AND);
  }

  if (!policy->AddRule(IpcTag::NTOPENSECTION, &open))
    return false;

  return true;
}

NTSTATUS SectionPolicy::OpenSectionAction(EvalResult eval_result,
                                          const ClientInfo& client_info,
                                          const base::string16& section_name,
                                          uint32_t desired_access,
                                          HANDLE* handle) {
  NtOpenSectionFunction NtOpenSection = nullptr;
  ResolveNTFunctionPtr("NtOpenSection", &NtOpenSection);

  // The only action supported is ASK_BROKER which means create the requested
  // section as specified.
  if (ASK_BROKER != eval_result)
    return false;

  HANDLE object_directory = nullptr;
  NTSTATUS status = GetBaseNamedObjectsDirectory(&object_directory);
  if (status != STATUS_SUCCESS)
    return status;

  UNICODE_STRING unicode_section_name = {};
  OBJECT_ATTRIBUTES object_attributes = {};
  InitObjectAttribs(section_name, OBJ_CASE_INSENSITIVE, object_directory,
                    &object_attributes, &unicode_section_name, nullptr);

  HANDLE local_handle = nullptr;
  status = NtOpenSection(&local_handle, desired_access, &object_attributes);
  if (!local_handle)
    return status;

  if (!::DuplicateHandle(::GetCurrentProcess(), local_handle,
                         client_info.process, handle, 0, false,
                         DUPLICATE_CLOSE_SOURCE | DUPLICATE_SAME_ACCESS)) {
    return STATUS_ACCESS_DENIED;
  }
  return status;
}

}  // namespace sandbox
