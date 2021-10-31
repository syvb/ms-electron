// Copyright (c) 2019 Microsoft Corporation.

#include "microsoft/src/media_blink/frame_sink_client.h"

#include <utility>

#include "base/check.h"
#include "base/strings/stringprintf.h"
#include "microsoft/src/media_blink/image_info.h"

namespace microsoft {

using LogLevel = blink::SkypeMediaPlayerClient;

FrameSinkClient::Frame::Frame(
    const ImageInfo& info_,
    const std::shared_ptr<const SharedMemory>& buffer_,
    const std::string& buffer_name_,
    uint32_t handle_)
    : info(info_),
      buffer(buffer_),
      buffer_name(buffer_name_),
      handle(handle_) {}

// Complex class/struct needs an explicit out-of-line copy constructor
FrameSinkClient::Frame::Frame(const Frame& other) = default;

// Complex class/struct needs an explicit out-of-line destructor
FrameSinkClient::Frame::~Frame() = default;

FrameSinkClient::FrameSinkClient(const std::string& name,
                                 FrameSinkClientDelegate* delegate)
    : name_(name), delegate_(delegate) {
  DCHECK(delegate);

  shm_ = SharedMemory::Open(name, true);
  flags_ = SharedMemory::Open(GetFlagsName(), false);

  if (!shm_ || !shm_->memory()) {
    Log(LogLevel::kLogLevelError, "FrameSinkClient - open shm failed");
    return;
  }

  if (shm_->mapped_size() < sizeof(frameSink::Header)) {
    Log(LogLevel::kLogLevelError, "FrameSinkClient - invalid shm size");
    return;
  }

  if (GetHeader()->version != kCurrentVersion) {
    Log(LogLevel::kLogLevelError, "FrameSinkClient - invalid shm version");
    return;
  }

  if (!flags_ || !flags_->memory()) {
    Log(LogLevel::kLogLevelError, "FrameSinkClient - open flags failed");
    return;
  }

  if (flags_->mapped_size() < sizeof(frameSink::Flags)) {
    Log(LogLevel::kLogLevelError, "FrameSinkClient - invalid flags size");
    return;
  }

  triple_buffer_.Attach(&GetFlags()->tripleBuffer);

  Log(LogLevel::kLogLevelInfo,
      base::StringPrintf("FrameSinkClient created - name: '%s'", name.c_str()));
}

FrameSinkClient::~FrameSinkClient() {
  Log(LogLevel::kLogLevelInfo, "FrameSinkClient destroyed");
}

bool FrameSinkClient::IsValid() const {
  return triple_buffer_.IsValid();
}

bool FrameSinkClient::SwapRead(bool background_rendering) {
  if (!triple_buffer_.IsValid() || !triple_buffer_.SwapRead())
    return false;

  auto index = triple_buffer_.GetRead();

  auto& frame = GetHeader()->frames.at(index);
  auto& buffer = buffers_.at(index);
  auto& buffer_id = buffer_ids_.at(index);

  if (buffer_id != frame.bufferId) {
    if (buffer_id) {
      Log(LogLevel::kLogLevelDebug,
          base::StringPrintf("FrameSinkClient - resetting, index: %u", index));
    }

    buffer.reset();
    buffer_id = 0;

    if (frame.bufferId) {
      auto name = GetFrameBufferName(frame.bufferId);
      auto new_buffer = SharedMemory::Open(name, true);

      if (!new_buffer) {
        Log(LogLevel::kLogLevelError,
            base::StringPrintf("FrameSinkClient - open failed, index: %u",
                               index));
        return false;
      }

      buffer = std::move(new_buffer);
      buffer_id = frame.bufferId;

      Log(LogLevel::kLogLevelDebug,
          base::StringPrintf("FrameSinkClient - index: %u, bufferId: %u", index,
                             buffer_id));
    }
  }

  GetFlags()->framesTotal++;

  if (background_rendering) {
    GetFlags()->framesInBackground++;
  }

  return true;
}

const FrameSinkClient::Frame FrameSinkClient::GetReadFrame() const {
  DCHECK(triple_buffer_.IsValid());

  auto index = triple_buffer_.GetRead();

  auto& frame = GetHeader()->frames.at(index);
  auto& buffer = buffers_.at(index);

  return Frame(frame.info, buffer, GetFrameBufferName(frame.bufferId),
               frame.handle);
}

void FrameSinkClient::OnFrameDropped() {
  GetFlags()->framesDropped++;
}

const frameSink::Header* FrameSinkClient::GetHeader() const {
  DCHECK(shm_);
  DCHECK(shm_->memory());

  return static_cast<const frameSink::Header*>(shm_->memory());
}

frameSink::Flags* FrameSinkClient::GetFlags() const {
  DCHECK(flags_);
  DCHECK(flags_->memory());

  return static_cast<frameSink::Flags*>(flags_->memory());
}

std::string FrameSinkClient::GetFrameBufferName(unsigned int buffer_id) const {
  return name_ + "-" + std::to_string(buffer_id);
}

std::string FrameSinkClient::GetFlagsName() const {
  return name_ + "-flags";
}

void FrameSinkClient::Log(blink::SkypeMediaPlayerClient::LogLevel level,
                          const std::string& message) {
  delegate_->OnFrameSinkClientLog(level, message);
}

}  // namespace microsoft
