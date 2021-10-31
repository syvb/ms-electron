// Copyright (c) 2019 Microsoft Corporation.

#ifndef SRC_GPU_COMMAND_BUFFER_SERVICE_UTILS_H_
#define SRC_GPU_COMMAND_BUFFER_SERVICE_UTILS_H_

#include <cstdint>

namespace microsoft {

uint32_t GetAlignmentWithoutPadding(int width,
                                    int height,
                                    int format,
                                    int type);

}  // namespace microsoft

#endif  // SRC_GPU_COMMAND_BUFFER_SERVICE_UTILS_H_
