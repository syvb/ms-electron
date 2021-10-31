// Copyright (c) 2019 Microsoft Corporation.

#include "microsoft/src/gpu/command_buffer/service/utils.h"

#include "gpu/command_buffer/common/gles2_cmd_utils.h"

namespace microsoft {

uint32_t GetAlignmentWithoutPadding(int width,
                                    int height,
                                    int format,
                                    int type) {
  static constexpr uint32_t alignments[] = {8, 4, 2, 1};

  for (auto alignment : alignments) {
    gpu::gles2::PixelStoreParams params;
    params.alignment = alignment;

    uint32_t size = 0;
    uint32_t padding = 0;

    if (gpu::gles2::GLES2Util::ComputeImageDataSizesES3(
            width, height, 1, format, type, params, &size, nullptr, nullptr,
            nullptr, &padding) &&
        padding == 0) {
      return alignment;
    }
  }

  return 0;
}

}  // namespace microsoft
