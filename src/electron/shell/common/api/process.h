// Copyright (c) 2020 Microsoft Corporation.

#ifndef SRC_ELECTRON_SHELL_COMMON_API_PROCESS_H_
#define SRC_ELECTRON_SHELL_COMMON_API_PROCESS_H_

#include "shell/common/gin_helper/dictionary.h"

namespace microsoft {

void ExtendProcessObject(gin_helper::Dictionary* process);

}  // namespace microsoft

#endif  // SRC_ELECTRON_SHELL_COMMON_API_PROCESS_H_
