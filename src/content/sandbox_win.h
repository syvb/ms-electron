// Copyright (c) 2019 Microsoft Corporation.

#ifndef SRC_CONTENT_SANDBOX_WIN_H_
#define SRC_CONTENT_SANDBOX_WIN_H_

#include "sandbox/win/src/sandbox_policy.h"

namespace microsoft {

bool AllowVideoRendererInSandbox(sandbox::TargetPolicy* policy);

}  // namespace microsoft

#endif  // SRC_CONTENT_SANDBOX_WIN_H_
