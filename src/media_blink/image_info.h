// Copyright (c) 2019 Microsoft Corporation.

#ifndef SRC_MEDIA_BLINK_IMAGE_INFO_H_
#define SRC_MEDIA_BLINK_IMAGE_INFO_H_

#include <array>
#include <atomic>

#include "microsoft/src/media_blink/triple_buffer.h"

// The memory layout of these data structures must match
// https://skype.visualstudio.com/SCC/_git/client-shared_electron_slimcore?path=%2Fsrc%2Futils%2FImageInfo.h&version=GBmaster

namespace microsoft {

struct CropInfo {
  uint32_t leftOffset = 0;
  uint32_t rightOffset = 0;
  uint32_t topOffset = 0;
  uint32_t bottomOffset = 0;
};

struct ImageInfo {
  uint32_t format = 0;
  uint32_t width = 0;
  uint32_t height = 0;
  uint32_t origWidth = 0;
  uint32_t origHeight = 0;
  uint32_t stride = 0;
  uint64_t timestamp = 0;
  bool mirror = false;
  CropInfo padding;
  CropInfo cropInfo;

  ImageInfo();
  ImageInfo(const ImageInfo&);
};

// The memory layout of these data structures must match
// https://skype.visualstudio.com/SCC/_git/client-shared_electron_slimcore?path=%2Fsrc%2Fslimcore%2Frendering%2FChromiumFrameSink.h&version=GBmaster

namespace frameSink {

struct Frame {
  ImageInfo info;
  uint32_t bufferId;
  uint32_t handle;
};

struct Header {
  uint32_t version;
  std::array<Frame, 3> frames;
};

struct Flags {
  TripleBuffer::atomic_t tripleBuffer;
  std::atomic<uint32_t> framesTotal;
  std::atomic<uint32_t> framesDropped;
  std::atomic<uint32_t> framesInBackground;

  Flags();
};

}  // namespace frameSink

static uint32_t kCurrentVersion = 1;

}  // namespace microsoft

#endif  // SRC_MEDIA_BLINK_IMAGE_INFO_H_
