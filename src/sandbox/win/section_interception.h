// Copyright (c) 2019 Microsoft Corporation.

#ifndef SRC_SANDBOX_WIN_SECTION_INTERCEPTION_H_
#define SRC_SANDBOX_WIN_SECTION_INTERCEPTION_H_

#include "sandbox/win/src/nt_internals.h"
#include "sandbox/win/src/sandbox_types.h"

namespace sandbox {

extern "C" {

typedef NTSTATUS(WINAPI* NtOpenSectionFunction)(
    PHANDLE SectionHandle,
    ACCESS_MASK DesiredAccess,
    POBJECT_ATTRIBUTES ObjectAttributes);

// Interceptor for NtOpenSection
SANDBOX_INTERCEPT NTSTATUS WINAPI
TargetNtOpenSection(NtOpenSectionFunction orig_OpenSection,
                    PHANDLE section_handle,
                    ACCESS_MASK desired_access,
                    POBJECT_ATTRIBUTES object_attributes);

}  // extern "C"

}  // namespace sandbox

#endif  // SRC_SANDBOX_WIN_SECTION_INTERCEPTION_H_
