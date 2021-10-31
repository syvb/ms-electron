// Copyright (c) 2020 Microsoft Corporation.

#include "microsoft/src/electron/shell/browser/crash_checker.h"

#include <memory>
#include <vector>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/files/file_util.h"
#include "base/no_destructor.h"
#include "base/path_service.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_restrictions.h"
#include "shell/common/electron_paths.h"
#include "third_party/crashpad/crashpad/client/crash_report_database.h"
#include "third_party/crashpad/crashpad/client/crashpad_client.h"
#include "third_party/crashpad/crashpad/client/settings.h"

constexpr int32_t MS_SESSION_STATE_MAGIC = 'ELCP';

namespace microsoft {

CrashChecker::CrashChecker() {
  CreateCache();
}

CrashChecker::~CrashChecker() {
  // CreateCache();
  // SendError("CrachChecker destroyed");
}

void CrashChecker::CreateCache() {
  base::FilePath crash_dir_path;
  base::PathService::Get(electron::DIR_CRASH_DUMPS, &crash_dir_path);

  base::ThreadRestrictions::ScopedAllowIO allow_io;
  base::CreateDirectory(crash_dir_path);

  crash_session_path_ =
      crash_dir_path.Append(FILE_PATH_LITERAL("session-state.dat"));

  bool write_state = true;
  if (base::PathExists(crash_session_path_)) {
    SessionState current_saved_session;
    base::ReadFile(crash_session_path_,
                   reinterpret_cast<char*>(&current_saved_session),
                   sizeof(current_saved_session));
    if (current_saved_session.header == MS_SESSION_STATE_MAGIC) {
      previous_session_state_ = current_saved_session;
    }
  }

  if (write_state) {
    time_t t = time(nullptr);
    struct tm timestruct;
#if defined(OS_WIN)
    gmtime_s(&timestruct, &t);
#else
    gmtime_r(&t, &timestruct);
#endif
    SessionState session_state = {};
    session_state.header = MS_SESSION_STATE_MAGIC;
    session_state.started_time = t;  // std::mktime(&timestruct);

    base::WriteFile(crash_session_path_,
                    reinterpret_cast<char*>(&session_state),
                    sizeof(session_state));
  }
}

bool CrashChecker::DidLastSessionCrash() {
  base::FilePath crash_dir_path;
  base::PathService::Get(electron::DIR_CRASH_DUMPS, &crash_dir_path);

  {
    base::ThreadRestrictions::ScopedAllowIO allow_io;
    if (!base::PathExists(crash_dir_path)) {
      return false;
    }
  }

  // Load crashpad database.
  std::unique_ptr<crashpad::CrashReportDatabase> database =
      crashpad::CrashReportDatabase::Initialize(crash_dir_path);
  DCHECK(database);

  std::vector<crashpad::CrashReportDatabase::Report> completed_reports;
  crashpad::CrashReportDatabase::OperationStatus status =
      database->GetCompletedReports(&completed_reports);
  if (status != crashpad::CrashReportDatabase::kNoError) {
    return false;
  }

  for (const crashpad::CrashReportDatabase::Report& completed_report :
       completed_reports) {
    if (completed_report.creation_time * 1000.0 >=
        previous_session_state_.started_time * 1000.0) {
      return true;
    }
  }

  return false;
}

}  // namespace microsoft
