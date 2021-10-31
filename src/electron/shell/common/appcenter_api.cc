// Copyright (c) 2019 Microsoft Corporation.
// https://docs.microsoft.com/en-us/appcenter/diagnostics/upload-crashes#upload-a-breakpad-crash-log-and-minidump

#include "microsoft/src/electron/shell/common/appcenter_api.h"

#include <ctime>
#include <map>
#include <memory>
#include <utility>
#include <vector>

#include "base/base64.h"
#include "base/files/file_util.h"
#include "base/guid.h"
#include "base/i18n/rtl.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/strings/string_split.h"
#include "base/strings/utf_string_conversions.h"
#include "base/system/sys_info.h"
#include "base/time/time_to_iso8601.h"
#include "base/values.h"
#include "electron/electron_version.h"
#include "microsoft/buildflags/buildflags.h"

#if defined(OS_MACOSX) || defined(OS_WIN)
#include "snapshot/module_snapshot.h"
#include "util/file/file_reader.h"
#include "util/misc/metrics.h"
#include "util/misc/uuid.h"
#include "util/net/http_body.h"
#include "util/net/http_multipart_builder.h"
#include "util/net/http_transport.h"
#include "util/net/url.h"
#endif

namespace microsoft {

namespace {

bool ParseAppCenterURL(const std::string& url,
                       std::string* app_secret,
                       std::string* instance_uid,
                       std::string* user_id,
                       std::string* session_id) {
  std::string result_scheme;

  size_t host_start;
  static constexpr const char kAppcenter[] = "appcenter://";
  if (url.compare(0, strlen(kAppcenter), kAppcenter) == 0) {
    host_start = strlen(kAppcenter);
  } else {
    LOG(ERROR) << "expecting appcenter url prefix";
    return false;
  }

  // Find the start of the parameters.
  size_t resource_start = url.find('?', host_start);
  if (resource_start == std::string::npos) {
    LOG(ERROR) << "expecting appcenter url parameters";
    return false;
  }

  auto rest = url.substr(resource_start + 1);
  for (const auto& cur : base::SplitStringUsingSubstr(
           rest, "&", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY)) {
    const auto kv = base::SplitStringUsingSubstr(
        cur, "=", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
    if (kv.size() == 2) {
      if (kv[0] == "aid") {
        *app_secret = kv[1];
      } else if (kv[0] == "iid") {
        *instance_uid = kv[1];
      } else if (kv[0] == "uid") {
        *user_id = kv[1];
      } else if (kv[0] == "sid") {
        *session_id = kv[1];
      }
    }
  }
  return true;
}

}  // namespace

// static
AppCenter* AppCenter::GetInstance() {
  return base::Singleton<AppCenter>::get();
}

AppCenter::AppCenter() : start_time_{base::Time::Now()} {}

AppCenter::~AppCenter() {}

void AppCenter::Initialize(const std::string& app_secret,
                           const std::string& instance_uid,
                           const std::string& user_id,
                           const std::string& session_id) {
  if (!base::IsValidGUID(app_secret)) {
    LOG(ERROR) << "invalid app secret";
    return;
  }
  app_secret_ = app_secret;

  instance_uid_ = instance_uid;
  if (instance_uid_.empty() || !base::IsValidGUID(instance_uid_)) {
    instance_uid_ = base::GenerateGUID();
  }

  user_id_ = user_id;
  if (user_id_.empty()) {
    user_id_ = base::GenerateGUID();
  }

  session_id_ = session_id;
  if (session_id_.empty() || !base::IsValidGUID(session_id_)) {
    session_id_ = base::GenerateGUID();
  }
}

void AppCenter::Initialize(const std::string& url) {
  std::string app_secret, instance_uid, user_id, session_id;

  // Parse appcenter parameter from url
  if (!ParseAppCenterURL(url, &app_secret, &instance_uid, &user_id,
                         &session_id)) {
    return;
  }

  Initialize(app_secret, instance_uid, user_id, session_id);
}

bool AppCenter::IsInitialized() const {
  return !app_secret_.empty();
}

void AppCenter::Set(const std::string& app_secret,
                    const std::string& instance_uid,
                    const std::string& user_id) {
  app_secret_ = app_secret;
  instance_uid_ = instance_uid;
  user_id_ = user_id;
}

base::Value AppCenter::GenerateDeviceInfo(const std::string& app_version) {
  base::Value dict(base::Value::Type::DICTIONARY);
  dict.SetKey("sdkName", base::Value("appcenter.custom"));
  dict.SetKey("sdkVersion", base::Value("2.4.1-SNAPSHOT"));
  dict.SetKey(
      "appNamespace",
      base::Value("com.microsoft.electron"));  // TODO(kykropyv): app-specific?
  dict.SetKey("appVersion", base::Value(app_version));

  std::string electron_version = ELECTRON_VERSION_STRING;
  // #if BUILDFLAG(MICROSOFT_ADD_BUILD_VERSION_TO_PROCESS_VERSIONS)
  //   electron_version += ";ms-";
  //   electron_version += MICROSOFT_BUILD_VERSION_STRING;
  // #endif
  dict.SetKey("appBuild",
              base::Value(electron_version));  // TODO(kykropyv): reiterate
  dict.SetKey("wrapperSdkName", base::Value("custom.ndk"));

  dict.SetKey(
      "screenSize",
      base::Value(
          "42x42"));  // do we need it on PC where there are multiple screens?

  std::string hwModel = base::SysInfo::HardwareModelName();
  if (hwModel.empty()) {
    hwModel = "Generic";
  }

  dict.SetKey("model", base::Value(hwModel));

  // dict.SetKey("osName", base::Value(base::SysInfo::OperatingSystemName()));
#if defined(OS_WIN)
  dict.SetKey("oemName", base::Value(hwModel));  // TODO(kykropyv): the same now
  dict.SetKey("osName", base::Value("Windows"));
#elif defined(OS_MACOSX)
  dict.SetKey("oemName", base::Value("Apple"));
  dict.SetKey("osName", base::Value("macOS"));
#else
  dict.SetKey("oemName", base::Value(hwModel));  // TODO(kykropyv): the same now
  dict.SetKey("osName", base::Value("Linux"));
#endif
  dict.SetKey("osVersion",
              base::Value(base::SysInfo::OperatingSystemVersion()));
  dict.SetKey(
      "osBuild",
      base::Value(base::SysInfo::OperatingSystemVersion()));  // TODO(kykropyv):
                                                              // the same now

  // dict.SetKey("locale", base::Value(base::i18n::GetConfiguredLocale()));
  dict.SetKey("locale",
              base::Value("en-US"));  // TODO(kykropyv): get from system
  dict.SetKey("carrierCountry",
              base::Value("us"));  // TODO(kykropyv): get from system

#if defined(OS_WIN)
  TIME_ZONE_INFORMATION tz{};
  if (GetTimeZoneInformation(&tz) != TIME_ZONE_ID_INVALID) {
    dict.SetKey("timeZoneOffset", base::Value(static_cast<int32_t>(-tz.Bias)));
  } else {
    dict.SetKey("timeZoneOffset", base::Value(0));
  }
#else
  time_t now = time(nullptr);
  tm time = {};
  localtime_r(&now, &time);
  dict.SetKey("timeZoneOffset",
              base::Value(static_cast<int32_t>(time.tm_gmtoff / 60)));
#endif
  return dict;
}

base::Value AppCenter::GenerateExceptionInfo(base::Value device_info,
                                             const std::string& id,
                                             const base::Time& time_stamp,
                                             const std::string& executable_name,
                                             int32_t pid) {
  base::Value dict(base::Value::Type::DICTIONARY);
  dict.SetKey("type", base::Value("managedError"));
  dict.SetKey("id", base::Value(id));
  dict.SetKey("sid", base::Value(session_id_));
  dict.SetKey("userId", base::Value(user_id_));
  dict.SetKey("processId", base::Value(pid));
  dict.SetKey("processName", base::Value(executable_name));
  dict.SetKey("fatal", base::Value(true));
  dict.SetKey("timestamp", base::Value(base::TimeToISO8601(time_stamp)));
  dict.SetKey("appLaunchTimestamp",
              base::Value(base::TimeToISO8601(start_time_)));

#if defined(ARCH_CPU_X86_64)
  dict.SetKey("architecture", base::Value("X64"));
#elif defined(ARCH_CPU_X86)
  dict.SetKey("architecture",
              base::Value("X86"));  // todo(kykropyv): not sure which value is
                                    // correct for x86 platfrom identifier here
#elif defined(ARCH_CPU_ARM64)
  dict.SetKey("architecture", base::Value("arm64"));
#elif defined(ARCH_CPU_ARMEL)
  dict.SetKey("architecture", base::Value("arm"));
#else
#error "Not implemented"
#endif

  dict.SetKey("device", std::move(device_info));

  base::Value exception(base::Value::Type::DICTIONARY);
  exception.SetKey("type", base::Value("minidump"));
  exception.SetKey("wrapperSdkName", base::Value("custom.ndk"));

  dict.SetKey("exception", std::move(exception));

  return dict;
}

base::Value AppCenter::GenerateMinidumpAttachment(
    base::Value device_info,
    const std::string& parent_guid,
    const base::Time& time_stamp,
    const std::string& minidump_b64) {
  base::Value dict(base::Value::Type::DICTIONARY);
  dict.SetKey("contentType", base::Value("application/octet-stream"));
  dict.SetKey("errorId", base::Value(parent_guid));
  dict.SetKey("fileName", base::Value("minidump.dmp"));
  dict.SetKey("id", base::Value(base::GenerateGUID()));
  dict.SetKey("sid", base::Value(session_id_));
  dict.SetKey("timestamp", base::Value(base::TimeToISO8601(time_stamp)));
  dict.SetKey("type", base::Value("errorAttachment"));
  dict.SetKey("device", std::move(device_info));
  dict.SetKey("data", base::Value(minidump_b64));

  return dict;
}

base::Value AppCenter::GenerateCrash(const std::string& executable_name,
                                     const std::string& version,
                                     int32_t pid,
                                     const std::string& minidump_b64) {
  auto device_info = GenerateDeviceInfo(version);
  auto id = base::GenerateGUID();
  auto time_stamp = base::Time::Now();

  base::Value logs(base::Value::Type::LIST);
  logs.Append(GenerateExceptionInfo(device_info.Clone(), id, time_stamp,
                                    executable_name, pid));
  logs.Append(GenerateMinidumpAttachment(std::move(device_info), id, time_stamp,
                                         minidump_b64));

  base::Value dict(base::Value::Type::DICTIONARY);
  dict.SetKey("logs", std::move(logs));

  return dict;
}

base::Optional<std::string> AppCenter::GenerateCrashJson(
    const std::string& executable_name,
    const std::string& version,
    int32_t pid,
    const std::string& minidump_b64) {
  std::string output;
  if (base::JSONWriter::Write(
          GenerateCrash(executable_name, version, pid, minidump_b64),
          &output)) {
    return output;
  }
  return base::nullopt;
}

#if defined(OS_MACOSX) || defined(OS_WIN)
bool AppCenter::CrashPadUploadReport(const std::string& payload,
                                     std::string* response_body) {
  std::unique_ptr<crashpad::HTTPBodyStream> body_stream{
      new crashpad::StringHTTPBodyStream{payload}};
  std::unique_ptr<crashpad::HTTPTransport> http_transport{
      crashpad::HTTPTransport::Create()};
  http_transport->SetHeader("App-Secret", app_secret_);
  http_transport->SetHeader("Install-ID", instance_uid_);
  http_transport->SetURL("https://in.appcenter.ms/logs?api-version=1.0.0");
  http_transport->SetMethod("POST");
  http_transport->SetTimeout(60.0);
  http_transport->SetBodyStream(std::move(body_stream));
  http_transport->SetHeader("Content-Type", "application/json");

  if (!http_transport->ExecuteSynchronously(response_body)) {
    return false;
  }
  return true;
}

bool AppCenter::CrashPadUploadReport(const std::string& executable_name,
                                     const std::string& version,
                                     int32_t pid,
                                     const base::StringPiece& minidump,
                                     std::string* response_body) {
  std::string minidump_b64;
  base::Base64Encode(minidump, &minidump_b64);
  auto json_payload =
      GenerateCrashJson(executable_name, version, pid, minidump_b64);
  if (json_payload) {
    return CrashPadUploadReport(json_payload.value(), response_body);
  }

  return false;
}

bool AppCenter::CrashPadUploadReport(const std::string& executable_name,
                                     const std::string& version,
                                     int32_t pid,
                                     crashpad::FileReader* reader,
                                     std::string* response_body) {
  if (app_secret_.empty()) {
    return false;
  }

  crashpad::FileOffset start_offset = reader->SeekGet();
  if (start_offset < 0) {
    return false;
  }

  if (!reader->Seek(0, SEEK_END)) {
    return false;
  }

  crashpad::FileOffset dump_size = reader->SeekGet();
  if (!reader->SeekSet(0)) {
    return false;
  }

  base::Optional<std::string> json_payload;
  {
    std::vector<char> dump_buffer;
    dump_buffer.resize(dump_size);
    if (!reader->ReadExactly(&dump_buffer.front(), dump_size)) {
      return false;
    }

    std::string minidump_b64;
    base::StringPiece minidump{
        reinterpret_cast<const char*>(&dump_buffer.front()), dump_size};
    base::Base64Encode(minidump, &minidump_b64);
    json_payload =
        GenerateCrashJson(executable_name, version, pid, minidump_b64);
  }

  if (json_payload) {
    return CrashPadUploadReport(json_payload.value(), response_body);
  }

  return false;
}
#endif  // defined(OS_MACOSX) || defined(OS_WIN)

}  // namespace microsoft
