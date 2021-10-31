// Copyright (c) 2020 Microsoft Corporation.

#ifndef SRC_ELECTRON_SHELL_BROWSER_CRASH_CHECKER_H_
#define SRC_ELECTRON_SHELL_BROWSER_CRASH_CHECKER_H_

#include <string>

#include "base/optional.h"
#include "shell/common/api/api.mojom.h"

#include "base/base64.h"
#include "base/files/file_util.h"
#include "base/guid.h"
#include "base/i18n/rtl.h"
#include "base/json/json_writer.h"
#include "base/strings/string_split.h"
#include "base/strings/utf_string_conversions.h"
#include "base/system/sys_info.h"
#include "base/time/time_to_iso8601.h"
#include "base/values.h"

namespace microsoft {

class CrashChecker {
 public:
  CrashChecker();
  ~CrashChecker();
  bool DidLastSessionCrash();

  struct SessionState {
    int32_t header;
    int64_t started_time;
  };

  base::FilePath crash_session_path_;
  SessionState previous_session_state_ = {};

 private:
  void CreateCache();
};

}  // namespace microsoft

#endif  // SRC_ELECTRON_SHELL_BROWSER_CRASH_CHECKER_H_
