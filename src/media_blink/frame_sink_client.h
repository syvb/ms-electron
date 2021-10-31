// Copyright (c) 2019 Microsoft Corporation.

#ifndef SRC_MEDIA_BLINK_FRAME_SINK_CLIENT_H_
#define SRC_MEDIA_BLINK_FRAME_SINK_CLIENT_H_

#include <array>
#include <atomic>
#include <memory>
#include <string>

#include "base/macros.h"
#include "microsoft/src/media_blink/image_info.h"
#include "microsoft/src/media_blink/triple_buffer.h"
#include "microsoft/src/skype_video/shared_memory.h"
#include "microsoft/src/third_party/blink/platform/skype_media_player_client.h"

namespace microsoft {

struct FrameSinkClientDelegate {
  virtual void OnFrameSinkClientLog(
      blink::SkypeMediaPlayerClient::LogLevel level,
      const std::string& message) = 0;
};

class FrameSinkClient {
 public:
  FrameSinkClient(const std::string& name, FrameSinkClientDelegate* delegate);
  ~FrameSinkClient();

  bool IsValid() const;
  bool SwapRead(bool background_rendering);

  struct Frame {
    const ImageInfo info;
    const std::shared_ptr<const SharedMemory> buffer;
    const std::string buffer_name;
    const uint32_t handle = 0;

    Frame(const ImageInfo& info_,
          const std::shared_ptr<const SharedMemory>& buffer_,
          const std::string& buffer_name_,
          uint32_t handle_);
    Frame(const Frame& other);
    ~Frame();
  };

  const Frame GetReadFrame() const;
  void OnFrameDropped();

 private:
  const frameSink::Header* GetHeader() const;
  frameSink::Flags* GetFlags() const;

  std::string GetFrameBufferName(unsigned int buffer_id) const;
  std::string GetFlagsName() const;

  void Log(blink::SkypeMediaPlayerClient::LogLevel level,
           const std::string& message);

  std::string name_;
  FrameSinkClientDelegate* delegate_ = nullptr;

  std::unique_ptr<SharedMemory> shm_;
  std::unique_ptr<SharedMemory> flags_;

  TripleBuffer triple_buffer_;
  std::array<std::shared_ptr<SharedMemory>, 3> buffers_;
  std::array<unsigned int, 3> buffer_ids_ = {};

  DISALLOW_COPY_AND_ASSIGN(FrameSinkClient);
};

}  // namespace microsoft

#endif  // SRC_MEDIA_BLINK_FRAME_SINK_CLIENT_H_
