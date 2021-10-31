// Copyright (c) 2020 Microsoft Corporation.

#include "microsoft/src/electron/shell/common/wer_api_win.h"

#include <windows.h>
// windows.h must be included first
#include <psapi.h>
#include <strsafe.h>
#include <werapi.h>

#include "base/command_line.h"
#include "base/numerics/safe_conversions.h"
#include "base/optional.h"
#include "base/scoped_generic.h"
#include "base/strings/utf_string_conversions.h"
#include "base/win/windows_version.h"
#include "content/public/common/content_switches.h"
#include "services/service_manager/sandbox/switches.h"
#include "shell/common/gin_helper/arguments.h"
#include "shell/common/gin_helper/dictionary.h"
#include "v8/include/v8.h"

#pragma comment(lib, "version.lib")
#pragma comment(lib, "wer.lib")
#pragma comment(lib, "psapi.lib")

namespace microsoft {

namespace {

constexpr auto EXCEPTION_MSVC = 0xe06d7363;  // 0xe0000000 | 'msc'

struct WerReportHandleTraits {
  static HREPORT InvalidValue() { return nullptr; }
  static void Free(HREPORT handle) { WerReportCloseHandle(handle); }
};

using WerReportHandle = base::ScopedGeneric<HREPORT, WerReportHandleTraits>;

HMODULE GetFaultingModule(PEXCEPTION_POINTERS exceptionPointers) {
  HMODULE module;
  if (!GetModuleHandleEx(
          GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
          static_cast<LPCTSTR>(
              exceptionPointers->ExceptionRecord->ExceptionAddress),
          &module)) {
    return nullptr;
  }

  return module;
}

void GetExceptionFriendlyName(PEXCEPTION_POINTERS exceptionPointers,
                              wchar_t* info,
                              size_t length) {
#define _CASE_NAME_W(x) L##x
#define _CASE_NAME(name)                               \
  case name:                                           \
    StringCchCopyW(info, length, _CASE_NAME_W(#name)); \
    break;

  switch (exceptionPointers->ExceptionRecord->ExceptionCode) {
    _CASE_NAME(EXCEPTION_ACCESS_VIOLATION);
    _CASE_NAME(EXCEPTION_ARRAY_BOUNDS_EXCEEDED);
    _CASE_NAME(EXCEPTION_BREAKPOINT);
    _CASE_NAME(EXCEPTION_DATATYPE_MISALIGNMENT);
    _CASE_NAME(EXCEPTION_FLT_DENORMAL_OPERAND);
    _CASE_NAME(EXCEPTION_FLT_DIVIDE_BY_ZERO);
    _CASE_NAME(EXCEPTION_FLT_INEXACT_RESULT);
    _CASE_NAME(EXCEPTION_FLT_INVALID_OPERATION);
    _CASE_NAME(EXCEPTION_FLT_OVERFLOW);
    _CASE_NAME(EXCEPTION_FLT_STACK_CHECK);
    _CASE_NAME(EXCEPTION_FLT_UNDERFLOW);
    _CASE_NAME(EXCEPTION_ILLEGAL_INSTRUCTION);
    _CASE_NAME(EXCEPTION_IN_PAGE_ERROR);
    _CASE_NAME(EXCEPTION_INT_DIVIDE_BY_ZERO);
    _CASE_NAME(EXCEPTION_INT_OVERFLOW);
    _CASE_NAME(EXCEPTION_INVALID_DISPOSITION);
    _CASE_NAME(EXCEPTION_NONCONTINUABLE_EXCEPTION);
    _CASE_NAME(EXCEPTION_PRIV_INSTRUCTION);
    _CASE_NAME(EXCEPTION_SINGLE_STEP);
    _CASE_NAME(EXCEPTION_STACK_OVERFLOW);
    _CASE_NAME(EXCEPTION_MSVC);
    _CASE_NAME(STATUS_INVALID_HANDLE);
    default:
      StringCchCopyW(info, length, L"Unknown exception");
  }
#undef _CASE_NAME_W
#undef _CASE_NAME
}

void GetExceptionDescription(PEXCEPTION_POINTERS exceptionPointers,
                             HMODULE faultingModule,
                             wchar_t* info,
                             size_t length) {
  wchar_t faultingModulePath[MAX_PATH + 1] = L"";
  GetModuleFileNameW(faultingModule, faultingModulePath,
                     _countof(faultingModulePath));

  MODULEINFO moduleInfo = {};
  GetModuleInformation(GetCurrentProcess(), faultingModule, &moduleInfo,
                       sizeof(moduleInfo));

  wchar_t faultingModuleName[_MAX_FNAME + _MAX_EXT + 1] = L"";
  wchar_t faultingModuleExt[_MAX_EXT + 1] = L"";
  _wsplitpath(faultingModulePath, nullptr, nullptr, faultingModuleName,
              faultingModuleExt);
  StringCchCatW(faultingModuleName, _countof(faultingModuleName),
                faultingModuleExt);

  StringCchPrintfW(info, length,
                   L"Exception 0x%x in module %s loaded at 0x%llx.",
                   exceptionPointers->ExceptionRecord->ExceptionCode,
                   faultingModuleName, moduleInfo.lpBaseOfDll);
}

bool GetModuleVersion(const wchar_t* modulePath,
                      wchar_t* version,
                      size_t length) {
  DWORD verHandle = 0;
  DWORD verSize =
      GetFileVersionInfoSizeExW(FILE_VER_GET_NEUTRAL, modulePath, &verHandle);
  if (verSize > 0) {
    BYTE* versionInfoBuff = reinterpret_cast<BYTE*>(_alloca(verSize));
    if (GetFileVersionInfoExW(FILE_VER_GET_NEUTRAL, modulePath, verHandle,
                              verSize, versionInfoBuff)) {
      UINT verInfoLen = 0;
      VS_FIXEDFILEINFO* verInfo = nullptr;
      if (VerQueryValueW(versionInfoBuff, L"\\",
                         reinterpret_cast<LPVOID*>(&verInfo), &verInfoLen)) {
        DWORD leftMost = HIWORD(verInfo->dwFileVersionMS);
        DWORD secondLeft = LOWORD(verInfo->dwFileVersionMS);
        DWORD secondRight = HIWORD(verInfo->dwFileVersionLS);
        DWORD rightMost = LOWORD(verInfo->dwFileVersionLS);

        StringCchPrintfW(version, length, L"%u.%u.%u.%u", leftMost, secondLeft,
                         secondRight, rightMost);
        return true;
      }
    }
  }
  StringCchCopyW(version, length, L"0.0.0.0");
  return false;
}

bool ReportFaultingModule(HREPORT reportHandle,
                          HMODULE faultingModule,
                          PEXCEPTION_POINTERS exceptionPointers) {
  // WER_P3 - name
  wchar_t faultingModulePath[MAX_PATH + 1] = L"";
  if (GetModuleFileNameW(faultingModule, faultingModulePath,
                         _countof(faultingModulePath)) == 0) {
    StringCchCopyW(faultingModulePath, _countof(faultingModulePath),
                   L"unknown");
  }

  wchar_t faultingModuleName[_MAX_FNAME + _MAX_EXT + 1] = L"";
  wchar_t faultingModuleExt[_MAX_EXT + 1] = L"";
  _wsplitpath(faultingModulePath, nullptr, nullptr, faultingModuleName,
              faultingModuleExt);
  StringCchCatW(faultingModuleName, _countof(faultingModuleName),
                faultingModuleExt);

  if (FAILED(WerReportSetParameter(reportHandle, WER_P3, nullptr,
                                   faultingModuleName))) {
    return false;
  }

  // WER_P4 - version
  wchar_t version[128] = L"";
  GetModuleVersion(faultingModulePath, version, _countof(version) - 1);
  if (FAILED(WerReportSetParameter(reportHandle, WER_P4, nullptr, version))) {
    return false;
  }

  // WER_P5 - timestamp
  wchar_t timeStamp[9] = L"";
  IMAGE_DOS_HEADER* DOSHeader =
      reinterpret_cast<IMAGE_DOS_HEADER*>(faultingModule);
  if (DOSHeader->e_magic == IMAGE_DOS_SIGNATURE) {
    IMAGE_NT_HEADERS* NTHeaders = reinterpret_cast<IMAGE_NT_HEADERS*>(
        (reinterpret_cast<BYTE*>(DOSHeader) + DOSHeader->e_lfanew));
    StringCchPrintfW(timeStamp, _countof(timeStamp), L"%08x",
                     NTHeaders->FileHeader.TimeDateStamp);
  } else {
    StringCchCopyW(timeStamp, _countof(timeStamp), L"00000000");
  }
  if (FAILED(WerReportSetParameter(reportHandle, WER_P5, nullptr, timeStamp))) {
    return false;
  }

  DWORD64 exceptionOffset =
      reinterpret_cast<DWORD64>(
          exceptionPointers->ExceptionRecord->ExceptionAddress) -
      reinterpret_cast<DWORD64>(faultingModule);
  wchar_t exceptionOffsetStr[32] = L"";
  StringCchPrintfW(exceptionOffsetStr, _countof(exceptionOffsetStr), L"%016llx",
                   exceptionOffset);

  if (FAILED(WerReportSetParameter(reportHandle, WER_P7, nullptr,
                                   exceptionOffsetStr))) {
    return false;
  }

  return true;
}

int CrashForException(EXCEPTION_POINTERS* exceptionPointers) {
  WER_EXCEPTION_INFORMATION exceptionInfo = {};
  exceptionInfo.pExceptionPointers = exceptionPointers;
  exceptionInfo.bClientPointers = FALSE;

  WER_REPORT_INFORMATION reportInformation = {};
  reportInformation.dwSize = sizeof(WER_REPORT_INFORMATION);
  reportInformation.hProcess = nullptr;

  GetModuleFileNameW(nullptr, reportInformation.wzApplicationPath,
                     _countof(reportInformation.wzApplicationPath));

  wchar_t appName[_MAX_FNAME + 1] = L"";
  wchar_t appExt[_MAX_EXT + 1] = L"";
  _wsplitpath(reportInformation.wzApplicationPath, nullptr, nullptr, appName,
              appExt);
  StringCchPrintfW(reportInformation.wzApplicationName,
                   _countof(reportInformation.wzApplicationName), L"%s%s",
                   appName, appExt);

  HMODULE faultingModule = GetFaultingModule(exceptionPointers);

  GetExceptionFriendlyName(exceptionPointers,
                           reportInformation.wzFriendlyEventName,
                           _countof(reportInformation.wzFriendlyEventName));
  GetExceptionDescription(exceptionPointers, faultingModule,
                          reportInformation.wzDescription,
                          _countof(reportInformation.wzDescription));

  WerReportHandle reportHandle;
  HREPORT reportHandleRaw = nullptr;
  if (FAILED(WerReportCreate(APPCRASH_EVENT, WerReportApplicationCrash,
                             &reportInformation, &reportHandleRaw))) {
    return EXCEPTION_EXECUTE_HANDLER;
  }

  reportHandle.reset(reportHandleRaw);

  // WER_P0 - Application executable name
  if (FAILED(WerReportSetParameter(reportHandle.get(), WER_P0, nullptr,
                                   reportInformation.wzApplicationName))) {
    return EXCEPTION_EXECUTE_HANDLER;
  }

  // WER_P1 - Application version
  wchar_t version[128] = L"";
  GetModuleVersion(reportInformation.wzApplicationPath, version,
                   _countof(version) - 1);
  if (FAILED(WerReportSetParameter(reportHandle.get(), WER_P1, nullptr,
                                   version))) {
    return EXCEPTION_EXECUTE_HANDLER;
  }

  // WER_P2 - Application timestamp
  wchar_t timeStamp[9] = L"";
  HMODULE executableModule =
      GetModuleHandleW(reportInformation.wzApplicationPath);
  IMAGE_DOS_HEADER* DOSHeader =
      reinterpret_cast<IMAGE_DOS_HEADER*>(executableModule);
  if (DOSHeader && DOSHeader->e_magic == IMAGE_DOS_SIGNATURE) {
    IMAGE_NT_HEADERS* NTHeaders = reinterpret_cast<IMAGE_NT_HEADERS*>(
        (reinterpret_cast<BYTE*>(DOSHeader) + DOSHeader->e_lfanew));
    StringCchPrintfW(timeStamp, _countof(timeStamp), L"%08x",
                     NTHeaders->FileHeader.TimeDateStamp);
  } else {
    StringCchCopyW(timeStamp, _countof(timeStamp), L"00000000");
  }

  if (FAILED(WerReportSetParameter(reportHandle.get(), WER_P2, nullptr,
                                   timeStamp))) {
    return EXCEPTION_EXECUTE_HANDLER;
  }

  // WER_P3-5
  if (!ReportFaultingModule(reportHandle.get(), faultingModule,
                            exceptionPointers)) {
    return EXCEPTION_EXECUTE_HANDLER;
  }

  // WER_P6 - ExceptionCode
  wchar_t exceptionCode[9] = L"";
  StringCchPrintfW(exceptionCode, _countof(exceptionCode), L"%08x",
                   exceptionPointers->ExceptionRecord->ExceptionCode);
  if (FAILED(WerReportSetParameter(reportHandle.get(), WER_P6, nullptr,
                                   exceptionCode))) {
    return EXCEPTION_EXECUTE_HANDLER;
  }

  // MiniDump
  if (FAILED(WerReportAddDump(reportHandle.get(), ::GetCurrentProcess(),
                              ::GetCurrentThread(), WerDumpTypeMiniDump,
                              &exceptionInfo, nullptr, 0))) {
    return EXCEPTION_EXECUTE_HANDLER;
  }

  DWORD submitOptions = WER_SUBMIT_HONOR_RESTART | WER_SUBMIT_HONOR_RECOVERY |
                        WER_SUBMIT_ADD_REGISTERED_DATA | WER_SUBMIT_QUEUE |
                        WER_SUBMIT_OUTOFPROCESS | WER_SUBMIT_NO_CLOSE_UI;

  // TODO(kykropyv): WerConsentApproved - privacy
  WER_SUBMIT_RESULT submitResult;
  HRESULT submitHResult = WerReportSubmit(
      reportHandle.get(), WerConsentApproved, submitOptions, &submitResult);

  if (FAILED(submitHResult)) {
    return EXCEPTION_EXECUTE_HANDLER;
  }

  reportHandle.reset();

  TerminateProcess(GetCurrentProcess(),
                   exceptionPointers->ExceptionRecord->ExceptionCode);
  return EXCEPTION_EXECUTE_HANDLER;
}

// from BreakPad sources
LONG WINAPI HandleException(EXCEPTION_POINTERS* exinfo) {
  // Ignore EXCEPTION_BREAKPOINT and EXCEPTION_SINGLE_STEP exceptions.  This
  // logic will short-circuit before calling WriteMinidumpOnHandlerThread,
  // allowing something else to handle the breakpoint without incurring the
  // overhead transitioning to and from the handler thread.  This behavior
  // can be overridden by calling ExceptionHandler::set_handle_debug_exceptions.
  // DWORD code = exinfo->ExceptionRecord->ExceptionCode;

  // bool is_debug_exception = (code == EXCEPTION_BREAKPOINT) || (code ==
  // EXCEPTION_SINGLE_STEP);  if (code == EXCEPTION_INVALID_HANDLE &&
  // current_handler->consume_invalid_handle_exceptions_) {
  //    return EXCEPTION_CONTINUE_EXECUTION;
  // }
  CrashForException(exinfo);

  return EXCEPTION_EXECUTE_HANDLER;
}

int HandleV8Exception(EXCEPTION_POINTERS* exinfo) {
  return HandleException(exinfo);
}

// from BreakPad sources
void HandleInvalidParameter(const wchar_t* expression,
                            const wchar_t* function,
                            const wchar_t* file,
                            unsigned int line,
                            uintptr_t reserved) {
  EXCEPTION_RECORD exception_record = {};
  CONTEXT exception_context = {};
  EXCEPTION_POINTERS exception_ptrs = {&exception_record, &exception_context};

  RtlCaptureContext(&exception_context);

  exception_record.ExceptionCode = STATUS_INVALID_PARAMETER;

  // We store pointers to the the expression and function strings,
  // and the line as exception parameters to make them easy to
  // access by the developer on the far side.
  exception_record.NumberParameters = 3;
  exception_record.ExceptionInformation[0] =
      reinterpret_cast<ULONG_PTR>(expression);
  exception_record.ExceptionInformation[1] = reinterpret_cast<ULONG_PTR>(file);
  exception_record.ExceptionInformation[2] = line;

  HandleException(&exception_ptrs);
}

// from BreakPad sources
void HandlePureVirtualCall() {
  EXCEPTION_RECORD exception_record = {};
  CONTEXT exception_context = {};
  EXCEPTION_POINTERS exception_ptrs = {&exception_record, &exception_context};

  RtlCaptureContext(&exception_context);

  exception_record.ExceptionCode = STATUS_NONCONTINUABLE_EXCEPTION;

  HandleException(&exception_ptrs);
}

bool EnableCustomHandler() {
  SetUnhandledExceptionFilter(HandleException);
  v8::V8::SetUnhandledExceptionCallback(HandleV8Exception);
  _set_invalid_parameter_handler(HandleInvalidParameter);
  _set_purecall_handler(HandlePureVirtualCall);

  return true;
}

bool EnableBackgroundReporting() {
  SetErrorMode(GetErrorMode() & ~SEM_NOGPFAULTERRORBOX);

  DWORD wer_flags = 0;
  HRESULT hr = WerGetFlags(GetCurrentProcess(), &wer_flags);
  if (FAILED(hr))
    return false;

  hr = WerSetFlags(wer_flags | WER_FAULT_REPORTING_NO_UI);
  return SUCCEEDED(hr);
}

bool IsSandboxed() {
  return !base::CommandLine::ForCurrentProcess()->HasSwitch(
      service_manager::switches::kNoSandbox);
}

bool Initialize() {
  if (base::win::GetVersion() < base::win::Version::WIN8) {
    if (!IsSandboxed()) {
      return EnableCustomHandler();
    } else {
      return false;
    }
  } else {
    return EnableBackgroundReporting();
  }
}

const char kEnableWER[] = "enable-wer";

struct Options {
  bool gpu_process = false;
  bool renderer_process = true;
  bool utility_process = true;
};

base::Optional<Options> init_options;

}  // namespace

bool WerInitialize(gin_helper::Arguments* args) {
  if (!args)
    return Initialize();

  init_options = Options();

  gin_helper::Dictionary options;
  if (args && args->GetNext(&options)) {
    options.Get("gpuProcess", &init_options->gpu_process);
    options.Get("rendererProcess", &init_options->renderer_process);
    options.Get("utilityProcess", &init_options->utility_process);
  }

  return Initialize();
}

void WerInitializeFromCommandLine() {
  auto* command_line = base::CommandLine::ForCurrentProcess();
  if (command_line->HasSwitch(kEnableWER)) {
    Initialize();
  }
}

bool WerRegisterFile(const std::string& file_path) {
  HRESULT hr = ::WerRegisterFile(base::UTF8ToWide(file_path).c_str(),
                                 WerRegFileTypeOther, WER_FILE_ANONYMOUS_DATA);
  return SUCCEEDED(hr);
}

bool WerUnregisterFile(const std::string& file_path) {
  HRESULT hr = ::WerUnregisterFile(base::UTF8ToWide(file_path).c_str());
  return SUCCEEDED(hr);
}

bool WerRegisterCustomMetadata(const std::string& key,
                               const std::string& value) {
  using WerRegisterCustomMetadataPtr = decltype(&::WerRegisterCustomMetadata);
  auto WerRegisterCustomMetadata =
      reinterpret_cast<WerRegisterCustomMetadataPtr>(GetProcAddress(
          GetModuleHandle(L"kernel32.dll"), "WerRegisterCustomMetadata"));

  if (!WerRegisterCustomMetadata) {
    return false;
  }

  HRESULT hr = WerRegisterCustomMetadata(base::UTF8ToWide(key).c_str(),
                                         base::UTF8ToWide(value).c_str());
  return SUCCEEDED(hr);
}

bool WerUnregisterCustomMetadata(const std::string& key) {
  using WerUnregisterCustomMetadataPtr =
      decltype(&::WerUnregisterCustomMetadata);
  auto WerUnregisterCustomMetadata =
      reinterpret_cast<WerUnregisterCustomMetadataPtr>(GetProcAddress(
          GetModuleHandle(L"kernel32.dll"), "WerUnregisterCustomMetadata"));

  if (!WerUnregisterCustomMetadata) {
    return false;
  }

  HRESULT hr = WerUnregisterCustomMetadata(base::UTF8ToWide(key).c_str());
  return SUCCEEDED(hr);
}

void AppendExtraCommandLineSwitches(base::CommandLine* command_line) {
  std::string process_type =
      command_line->GetSwitchValueASCII(::switches::kProcessType);

  if ((process_type == ::switches::kGpuProcess && init_options &&
       init_options->gpu_process) ||
      (process_type == ::switches::kRendererProcess && init_options &&
       init_options->renderer_process) ||
      (process_type == ::switches::kUtilityProcess && init_options &&
       init_options->utility_process)) {
    command_line->AppendSwitch(kEnableWER);
  }
}

}  // namespace microsoft
