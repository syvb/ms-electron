// Copyright (c) 2019 Microsoft Corporation.

#include "microsoft/src/electron/shell/common/api/watchdog.h"

#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/files/file_util.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/path_service.h"
#include "base/power_monitor/power_monitor.h"
#include "base/strings/stringprintf.h"
#include "base/threading/thread_task_runner_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/renderer/render_frame.h"
#include "services/service_manager/public/cpp/interface_provider.h"
#include "shell/browser/api/electron_api_web_contents.h"
#include "shell/common/api/api.mojom.h"
#include "shell/common/electron_paths.h"
#include "shell/common/gin_helper/dictionary.h"
#include "third_party/blink/public/web/web_local_frame.h"

namespace microsoft {

static content::RenderFrame* GetCurrentRenderFrame() {
  auto* frame = blink::WebLocalFrame::FrameForCurrentContext();
  if (!frame)
    return nullptr;

  return content::RenderFrame::FromWebFrame(frame);
}

static base::FilePath GetLogFilePath(int process_id, bool create_dir = false) {
  base::FilePath user_dir;
  if (!base::PathService::Get(electron::DIR_USER_DATA, &user_dir)) {
    LOG(ERROR) << "base::PathService::Get() failed";
    return base::FilePath();
  }

  auto watchdog_dir = user_dir.AppendASCII("watchdog");

  if (create_dir) {
    base::ThreadRestrictions::ScopedAllowIO allow_io;
    if (!base::CreateDirectory(watchdog_dir)) {
      LOG(ERROR) << "base::CreateDirectory() failed";
      return base::FilePath();
    }
  }

  return watchdog_dir.AppendASCII(
      base::StringPrintf("watchdog_%d.json", process_id));
}

static base::File CreateLogFileImpl(int process_id) {
  auto file_path = GetLogFilePath(process_id, true);
  if (file_path.empty()) {
    return base::File();
  }

  base::ThreadRestrictions::ScopedAllowIO allow_io;
  return base::File(file_path,
                    base::File::FLAG_CREATE_ALWAYS | base::File::FLAG_WRITE);
}

static void DeleteLogFileImpl(int process_id) {
  auto file_path = GetLogFilePath(process_id);
  if (file_path.empty()) {
    LOG(ERROR) << "Watchdog::GetLogFilePath() failed";
    return;
  }

  base::ThreadRestrictions::ScopedAllowIO allow_io;
  base::DeleteFile(file_path, false);
}

static base::File OpenLogFile() {
  auto* render_frame = GetCurrentRenderFrame();
  if (!render_frame) {
    LOG(ERROR) << "GetCurrentRenderFrame() failed";
    return base::File();
  }

  mojo::Remote<electron::mojom::ElectronBrowser> electron_browser_remote;
  render_frame->GetRemoteInterfaces()->GetInterface(
      electron_browser_remote.BindNewPipeAndPassReceiver());

  mojo::ScopedHandle file_handle;
  if (!electron_browser_remote->WatchdogOpenLogFile(&file_handle)) {
    LOG(ERROR) << "WatchdogOpenLogFile() failed";
    return base::File();
  }

  base::ThreadRestrictions::ScopedAllowIO allow_io;

  base::PlatformFile platform_file;
  auto mojo_result =
      mojo::UnwrapPlatformFile(std::move(file_handle), &platform_file);

  if (mojo_result != MOJO_RESULT_OK) {
    LOG(ERROR) << "Unable to get the file handle from mojo.";
    return base::File();
  }

  return base::File(platform_file);
}

static void DeleteLogFile() {
  auto* render_frame = GetCurrentRenderFrame();
  if (!render_frame) {
    LOG(ERROR) << "GetCurrentRenderFrame() failed";
    return;
  }

  mojo::Remote<electron::mojom::ElectronBrowser> electron_browser_remote;
  render_frame->GetRemoteInterfaces()->GetInterface(
      electron_browser_remote.BindNewPipeAndPassReceiver());

  electron_browser_remote->WatchdogDeleteLogFile();
}

static std::unique_ptr<Watchdog> watchdog;

// static
void Watchdog::Start(unsigned int timeout, gin_helper::Arguments* args) {
  Options options;
  gin_helper::Dictionary obj;
  if (args->GetNext(&obj)) {
    obj.Get("pingInterval", &options.ping_interval);
    obj.Get("usePowerMonitor", &options.use_power_monitor);
  }

  if (watchdog) {
    Stop();
  }

  LOG(INFO) << "Creating watchdog";
  watchdog = std::make_unique<Watchdog>(timeout, options);
}

// static
void Watchdog::Stop() {
  LOG(INFO) << "Destroying watchdog";
  watchdog.reset();
}

Watchdog::BlockingApi::BlockingApi(const std::string& api) {
  if (watchdog) {
    watchdog->blocking_api_ = api;
  }
}

Watchdog::BlockingApi::~BlockingApi() {
  if (watchdog) {
    watchdog->blocking_api_ = std::string();
  }
}

Watchdog::Service::Service(electron::api::WebContents* web_contents)
    : web_contents_(web_contents) {
  DCHECK(web_contents_);
}

base::Value Watchdog::Service::GetMetadata() {
  auto file_path = GetLogFilePath(web_contents_->GetProcessID());
  if (file_path.empty()) {
    return base::Value();
  }

  base::ThreadRestrictions::ScopedAllowIO allow_io;

  std::string json;
  if (!base::ReadFileToString(file_path, &json)) {
    return base::Value();
  }

  base::DeleteFile(file_path, false);

  auto dict = base::JSONReader::Read(json);
  return dict ? dict->Clone() : base::Value();
}

mojo::ScopedHandle Watchdog::Service::HandleOpenLogFile() {
  return mojo::WrapPlatformFile(
      CreateLogFileImpl(web_contents_->GetProcessID()).TakePlatformFile());
}

void Watchdog::Service::HandleDeleteLogFile() {
  return DeleteLogFileImpl(web_contents_->GetProcessID());
}

// static
void Watchdog::RenderProcessExited(int process_id) {
  DeleteLogFileImpl(process_id);
}

// called on the main sequence
Watchdog::Watchdog(unsigned int timeout, const Options& options)
    : timeout_(base::TimeDelta::FromMilliseconds(timeout)),
      options_(options),
      log_file_(OpenLogFile()),
      done_event_(base::WaitableEvent::ResetPolicy::AUTOMATIC,
                  base::WaitableEvent::InitialState::NOT_SIGNALED),
      resumed_event_(base::WaitableEvent::ResetPolicy::MANUAL,
                     base::WaitableEvent::InitialState::SIGNALED),
      task_runner_(base::ThreadTaskRunnerHandle::Get()),
      weak_factory_(this) {
  StartPinging();
  StartPowerObserver();
  base::PlatformThread::Create(0, this, &thread_handle_);
}

// called on the main sequence
Watchdog::~Watchdog() {
  StopPowerObserver();
  done_event_.Signal();
  base::PlatformThread::Join(thread_handle_);
  log_file_.Close();
  DeleteLogFile();
}

// called on the main sequence
void Watchdog::StartPinging() {
  if (options_.ping_interval) {
    LOG(INFO) << "Watchdog pinging every " << options_.ping_interval << " ms";
    ping_timer_.Start(
        FROM_HERE, base::TimeDelta::FromMilliseconds(options_.ping_interval),
        base::Bind(&Watchdog::UpdateLastPing, weak_factory_.GetWeakPtr()));
    UpdateLastPing();
  } else {
    LOG(INFO) << "Watchdog pinging not enabled";
  }
}

// called on the main sequence
void Watchdog::StartPowerObserver() {
  if (options_.use_power_monitor) {
    LOG(INFO) << "Watchdog using PowerMonitor";
    DCHECK(base::PowerMonitor::IsInitialized());
    base::PowerMonitor::AddObserver(this);
  } else {
    LOG(INFO) << "Watchdog not using PowerMonitor";
  }
}

// called on the main sequence
void Watchdog::StopPowerObserver() {
  if (options_.use_power_monitor) {
    base::PowerMonitor::RemoveObserver(this);
  }
}

// called on the main sequence
void Watchdog::StopPinging() {
  ping_timer_.Stop();
}

// called on watchdog thread
void Watchdog::Crash() {
#if defined(OS_WIN)
  RaiseFailFastException(
      nullptr, nullptr,
      FAIL_FAST_GENERATE_EXCEPTION_ADDRESS | FAIL_FAST_NO_HARD_ERROR_DLG);
#elif defined(OS_MACOSX)
  RaiseException();
#endif

  struct DummyClass {
    bool crash;
  };

  static_cast<DummyClass*>(nullptr)->crash = true;
}

// called on the main sequence
void Watchdog::OnResume() {
  LOG(INFO) << "Watchdog resumed";
  SetResponsive();
  StartPinging();
  resumed_event_.Signal();
}

// called on the main sequence
void Watchdog::OnSuspend() {
  LOG(INFO) << "Watchdog suspended";
  resumed_event_.Reset();
  StopPinging();
}

// called on watchdog thread
void Watchdog::Log() {
  if (!log_file_.IsValid())
    return;

  base::Value dict(base::Value::Type::DICTIONARY);

  // blocking_api_ access is not synchronized. we're about to crash here
  // anyway and we don't want to risk deadlocking the main thread by waiting
  // on some lock in every BlockingApi::BlockingApi() call

  dict.SetKey("blockingApi", base::Value(blocking_api_));
  dict.SetKey("timeout", base::Value(timeout_.InMillisecondsF()));

  if (!last_ping_.is_null()) {
    dict.SetKey("lastPing", base::Value(last_ping_.ToJsTime()));
  }

  std::string json;
  base::JSONWriter::Write(dict, &json);

  log_file_.WriteAtCurrentPos(json.data(), json.size());
  log_file_.Flush();
}

// called on the main sequence
void Watchdog::SetResponsive() {
  unresponsive_.clear();
}

// called on the main sequence
void Watchdog::UpdateLastPing() {
  VLOG(1) << "Watchdog ping";
  last_ping_ = base::Time::NowFromSystemTime();
}

void Watchdog::ThreadMain() {
  base::PlatformThread::SetName("Watchdog");

  while (!done_event_.TimedWait(timeout_)) {
    resumed_event_.Wait();

    if (unresponsive_.test_and_set()) {
      Log();
      Crash();
    }

    VLOG(1) << "Watchdog posting to task_runner";

    DCHECK(task_runner_);
    task_runner_->PostTask(FROM_HERE, base::Bind(&Watchdog::SetResponsive,
                                                 weak_factory_.GetWeakPtr()));
  }
}

}  // namespace microsoft
