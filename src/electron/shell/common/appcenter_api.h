// Copyright (c) 2019 Microsoft Corporation.

#ifndef SRC_ELECTRON_SHELL_COMMON_APPCENTER_API_H_
#define SRC_ELECTRON_SHELL_COMMON_APPCENTER_API_H_

#include <cstddef>
#include <map>
#include <string>

#include "base/memory/singleton.h"
#include "base/optional.h"
#include "base/time/time.h"
#include "base/values.h"
#include "build/build_config.h"

#if defined(OS_MACOSX) || defined(OS_WIN)
namespace crashpad {
class FileReader;
}  // namespace crashpad
#endif

namespace microsoft {

class AppCenter {
 public:
  AppCenter();
  ~AppCenter();

  void Initialize(const std::string& app_secret,
                  const std::string& instance_uid,
                  const std::string& user_id,
                  const std::string& session_id);
  void Initialize(const std::string& url);
  void Set(const std::string& app_secret,
           const std::string& instance_uid,
           const std::string& user_id);

  bool IsInitialized() const;

  base::Value GenerateDeviceInfo(const std::string& app_version);
  base::Value GenerateExceptionInfo(base::Value device_info,
                                    const std::string& id,
                                    const base::Time& time_stamp,
                                    const std::string& executable_name,
                                    int32_t pid);
  base::Value GenerateMinidumpAttachment(base::Value device_info,
                                         const std::string& parent_guid,
                                         const base::Time& time_stamp,
                                         const std::string& minidump_b64);

  base::Value GenerateCrash(const std::string& executable_name,
                            const std::string& version,
                            int32_t pid,
                            const std::string& minidump_b64);

  base::Optional<std::string> GenerateCrashJson(
      const std::string& executable_name,
      const std::string& version,
      int32_t pid,
      const std::string& minidump_b64);

#if defined(OS_MACOSX) || defined(OS_WIN)
  // todo(kykropyv): remove unused later
  bool CrashPadUploadReport(const std::string& payload,
                            std::string* response_body);
  bool CrashPadUploadReport(const std::string& executable_name,
                            const std::string& version,
                            int32_t pid,
                            const base::StringPiece& minidump,
                            std::string* response_body);
  bool CrashPadUploadReport(const std::string& executable_name,
                            const std::string& version,
                            int32_t pid,
                            crashpad::FileReader* reader,
                            std::string* response_body);
#endif  // defined(OS_MACOSX) || defined(OS_WIN)

  static AppCenter* GetInstance();

  const std::string& app_secret() { return app_secret_; }
  const std::string& instance_uid() { return instance_uid_; }

 private:
  friend struct base::DefaultSingletonTraits<AppCenter>;

  std::string app_secret_;
  std::string instance_uid_;
  std::string user_id_;
  std::string session_id_;
  base::Time start_time_;
};

}  // namespace microsoft

#endif  // SRC_ELECTRON_SHELL_COMMON_APPCENTER_API_H_
