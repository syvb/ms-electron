// Copyright (c) 2019 Microsoft Corporation.

#ifndef SRC_SANDBOX_WIN_SECTION_POLICY_H_
#define SRC_SANDBOX_WIN_SECTION_POLICY_H_

#include <cstdint>
#include <string>

#include "base/strings/string16.h"
#include "sandbox/win/src/crosscall_server.h"
#include "sandbox/win/src/nt_internals.h"
#include "sandbox/win/src/policy_low_level.h"
#include "sandbox/win/src/sandbox_policy.h"

namespace sandbox {

enum EvalResult;

// This class centralizes most of the knowledge related to sections policy
class SectionPolicy {
 public:
  // Creates the required low-level policy rules to evaluate a high-level
  // policy rule for section calls, in particular open or create actions.
  // name is the section object name, semantics is the desired semantics for the
  // open or create and policy is the policy generator to which the rules are
  // going to be added.
  static bool GenerateRules(const wchar_t* name,
                            TargetPolicy::Semantics semantics,
                            LowLevelPolicy* policy);

  // Performs the desired policy action on a request.
  // client_info is the target process that is making the request and
  // eval_result is the desired policy action to accomplish.
  static NTSTATUS OpenSectionAction(EvalResult eval_result,
                                    const ClientInfo& client_info,
                                    const base::string16& section_name,
                                    uint32_t desired_access,
                                    HANDLE* handle);
};

}  // namespace sandbox

#endif  // SRC_SANDBOX_WIN_SECTION_POLICY_H_
