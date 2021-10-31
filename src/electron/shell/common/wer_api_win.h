// Copyright (c) 2020 Microsoft Corporation.

#ifndef SRC_ELECTRON_SHELL_COMMON_WER_API_WIN_H_
#define SRC_ELECTRON_SHELL_COMMON_WER_API_WIN_H_

#include <string>

namespace base {
class CommandLine;
}

namespace gin_helper {
class Arguments;
}

namespace microsoft {

bool WerInitialize(gin_helper::Arguments* args = nullptr);
void WerInitializeFromCommandLine();

bool WerRegisterFile(const std::string& file_path);
bool WerUnregisterFile(const std::string& file_path);

bool WerRegisterCustomMetadata(const std::string& key,
                               const std::string& value);
bool WerUnregisterCustomMetadata(const std::string& key);

void AppendExtraCommandLineSwitches(base::CommandLine* command_line);

}  // namespace microsoft

#endif  // SRC_ELECTRON_SHELL_COMMON_WER_API_WIN_H_
