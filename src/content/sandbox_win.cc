// Copyright (c) 2019 Microsoft Corporation.

#include "microsoft/src/content/sandbox_win.h"

namespace microsoft {

bool AllowVideoRendererInSandbox(sandbox::TargetPolicy* policy) {
  sandbox::ResultCode result;
  result = policy->AddRule(sandbox::TargetPolicy::SUBSYS_SYNC,
                           sandbox::TargetPolicy::EVENTS_ALLOW_ANY,
                           L"Local\\VideoRenderer::EventObject::*");

  if (result != sandbox::SBOX_ALL_OK)
    return false;

  result = policy->AddRule(sandbox::TargetPolicy::SUBSYS_SECTIONS,
                           sandbox::TargetPolicy::SECTIONS_ALLOW_ANY,
                           L"Local\\VideoRenderer::SharedMemory::*-flags");

  if (result != sandbox::SBOX_ALL_OK)
    return false;

  result = policy->AddRule(sandbox::TargetPolicy::SUBSYS_SECTIONS,
                           sandbox::TargetPolicy::SECTIONS_ALLOW_READONLY,
                           L"Local\\VideoRenderer::SharedMemory::*");

  if (result != sandbox::SBOX_ALL_OK)
    return false;

  return true;
}

}  // namespace microsoft
