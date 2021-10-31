// Copyright (c) 2019 Microsoft Corporation.

#ifndef SRC_ELECTRON_SHELL_COMMON_API_WATCHDOG_H_
#define SRC_ELECTRON_SHELL_COMMON_API_WATCHDOG_H_

#include <atomic>
#include <string>

#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/power_monitor/power_observer.h"
#include "base/single_thread_task_runner.h"
#include "base/synchronization/waitable_event.h"
#include "base/threading/platform_thread.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "base/values.h"
#include "mojo/public/cpp/system/platform_handle.h"
#include "shell/common/gin_helper/arguments.h"

namespace content {
class RenderFrameHost;
}

namespace electron {
namespace api {
class WebContents;
}
}  // namespace electron

namespace IPC {
class Message;
}

namespace microsoft {

class Watchdog : public base::PlatformThread::Delegate,
                 public base::PowerObserver {
 public:
  static void Start(unsigned int timeout, gin_helper::Arguments* args);
  static void Stop();

  class BlockingApi {
   public:
    explicit BlockingApi(const std::string& api);
    ~BlockingApi();

    DISALLOW_COPY_AND_ASSIGN(BlockingApi);
  };

  class Service {
   public:
    explicit Service(electron::api::WebContents* web_contents);

    base::Value GetMetadata();

    mojo::ScopedHandle HandleOpenLogFile();
    void HandleDeleteLogFile();

   private:
    electron::api::WebContents* web_contents_ = nullptr;

    DISALLOW_COPY_AND_ASSIGN(Service);
  };

  static void RenderProcessExited(int process_id);

  struct Options {
    unsigned int ping_interval = 0;
    bool use_power_monitor = false;
  };

  explicit Watchdog(unsigned int timeout, const Options& options);
  ~Watchdog() override;

 private:
#if defined(OS_MACOSX)
  void RaiseException();
#endif
  void Crash();
  void Log();
  void SetResponsive();
  void StartPinging();
  void StartPowerObserver();
  void StopPinging();
  void StopPowerObserver();
  void UpdateLastPing();

  // base::PlatformThread::Delegate
  void ThreadMain() override;

  // base::PowerObserver
  void OnSuspend() override;
  void OnResume() override;

  const base::TimeDelta timeout_;
  const Options options_;

  base::File log_file_;
  base::PlatformThreadHandle thread_handle_;
  std::atomic_flag unresponsive_ = ATOMIC_FLAG_INIT;
  base::WaitableEvent done_event_;
  base::WaitableEvent resumed_event_;
  std::string blocking_api_;
  base::RepeatingTimer ping_timer_;
  base::Time last_ping_;

  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;
  base::WeakPtrFactory<Watchdog> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(Watchdog);
};

}  // namespace microsoft

#endif  // SRC_ELECTRON_SHELL_COMMON_API_WATCHDOG_H_
