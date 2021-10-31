// Copyright (c) 2019 Microsoft Corporation.

#include "microsoft/src/media_blink/skypevideo_webmediaplayer.h"

#include <utility>
#include <vector>

#include "base/strings/stringprintf.h"
#include "cc/layers/video_layer.h"
#include "components/viz/common/frame_sinks/begin_frame_args.h"
#include "components/viz/common/gpu/context_provider.h"
#include "content/renderer/render_thread_impl.h"
#include "media/base/bind_to_current_loop.h"
#include "services/viz/public/cpp/gpu/context_provider_command_buffer.h"
#include "third_party/blink/public/platform/web_media_player_client.h"

namespace microsoft {

using LogLevel = blink::SkypeMediaPlayerClient;

constexpr uint32_t FOURCC(uint32_t a, uint32_t b, uint32_t c, uint32_t d) {
  return (a) | (b << 8) | (c << 16) | (d << 24);
}

constexpr auto FOURCC_BGRA = FOURCC('B', 'G', 'R', 'A');
constexpr auto FOURCC_I420 = FOURCC('I', '4', '2', '0');
constexpr auto FOURCC_NV12 = FOURCC('N', 'V', '1', '2');

SkypeVideoWebMediaPlayer::SkypeVideoWebMediaPlayer(
    blink::WebMediaPlayerClient* client,
    const GURL& gurl,
    scoped_refptr<base::SingleThreadTaskRunner> compositor_task_runner)
    : client_(client),
      compositor_task_runner_(compositor_task_runner
                                  ? compositor_task_runner
                                  : base::ThreadTaskRunnerHandle::Get()),
      main_task_runner_(base::ThreadTaskRunnerHandle::Get()),
      compositor_(
          new media::VideoFrameCompositor(compositor_task_runner_, nullptr)) {
  DCHECK(client_);

  LogEvent(LogLevel::kLogLevelInfo, "SkypeVideoWebMediaPlayer created");

  OnContextLost();

  frame_sink_ = std::make_unique<FrameSinkClient>(gurl.GetContent(), this);

  if (!frame_sink_->IsValid()) {
    LogEvent(LogLevel::kLogLevelError, "FrameSinkClient failed");
    main_task_runner_->PostTask(
        FROM_HERE, base::Bind(&SkypeVideoWebMediaPlayer::SetNetworkState,
                              weak_factory_.GetWeakPtr(),
                              blink::WebMediaPlayer::kNetworkStateFormatError));
  }

  compositor_->Start(this);
}

SkypeVideoWebMediaPlayer::~SkypeVideoWebMediaPlayer() {
  DCHECK(thread_checker_.CalledOnValidThread());

  SetContextProvider(nullptr);

  // TODO(miburda): uninitialize only if initialized
  compositor_->Stop();

  client_->SetCcLayer(nullptr);
  if (video_layer_)
    video_layer_->StopUsingProvider();
  compositor_task_runner_->DeleteSoon(FROM_HERE, compositor_);

  LogEvent(LogLevel::kLogLevelInfo, "SkypeVideoWebMediaPlayer destroyed");
}

blink::WebMediaPlayer::LoadTiming SkypeVideoWebMediaPlayer::Load(
    LoadType,
    const blink::WebMediaPlayerSource&,
    CorsMode) {
  DCHECK(thread_checker_.CalledOnValidThread());
  return kImmediate;
}

void SkypeVideoWebMediaPlayer::Play() {
  DCHECK(thread_checker_.CalledOnValidThread());
  // not implemented
}

void SkypeVideoWebMediaPlayer::Pause() {
  DCHECK(thread_checker_.CalledOnValidThread());
  // not implemented
}

void SkypeVideoWebMediaPlayer::Seek(double seconds) {
  DCHECK(thread_checker_.CalledOnValidThread());
  // not implemented
}

void SkypeVideoWebMediaPlayer::SetRate(double) {
  DCHECK(thread_checker_.CalledOnValidThread());
  // not implemented
}

void SkypeVideoWebMediaPlayer::SetVolume(double) {
  DCHECK(thread_checker_.CalledOnValidThread());
  // not implemented
}

void SkypeVideoWebMediaPlayer::SetLatencyHint(double seconds) {
  DCHECK(thread_checker_.CalledOnValidThread());
  // not implemented
}

void SkypeVideoWebMediaPlayer::SetPreservesPitch(bool preserves_pitch) {
  DCHECK(thread_checker_.CalledOnValidThread());
  // not implemented
}

void SkypeVideoWebMediaPlayer::OnRequestPictureInPicture() {
  DCHECK(thread_checker_.CalledOnValidThread());
  // not implemented
}

void SkypeVideoWebMediaPlayer::OnPictureInPictureAvailabilityChanged(
    bool available) {
  DCHECK(thread_checker_.CalledOnValidThread());
  // not implemented
}

blink::WebTimeRanges SkypeVideoWebMediaPlayer::Buffered() const {
  DCHECK(thread_checker_.CalledOnValidThread());
  return blink::WebTimeRanges();
}

blink::WebTimeRanges SkypeVideoWebMediaPlayer::Seekable() const {
  DCHECK(thread_checker_.CalledOnValidThread());
  return blink::WebTimeRanges();
}

void SkypeVideoWebMediaPlayer::SetSinkId(const blink::WebString& sink_id,
                                         blink::WebSetSinkIdCompleteCallback) {
  DCHECK(thread_checker_.CalledOnValidThread());
  // not implemented
}

bool SkypeVideoWebMediaPlayer::HasVideo() const {
  DCHECK(thread_checker_.CalledOnValidThread());
  return true;
}

bool SkypeVideoWebMediaPlayer::HasAudio() const {
  DCHECK(thread_checker_.CalledOnValidThread());
  return false;
}

gfx::Size SkypeVideoWebMediaPlayer::NaturalSize() const {
  DCHECK(thread_checker_.CalledOnValidThread());
  return video_size_fake_;
}

gfx::Size SkypeVideoWebMediaPlayer::VisibleSize() const {
  DCHECK(thread_checker_.CalledOnValidThread());
  auto video_frame = compositor_->GetCurrentFrame();
  if (!video_frame)
    return gfx::Size();

  return video_frame->visible_rect().size();
}

bool SkypeVideoWebMediaPlayer::Paused() const {
  DCHECK(thread_checker_.CalledOnValidThread());
  return false;
}

bool SkypeVideoWebMediaPlayer::Seeking() const {
  DCHECK(thread_checker_.CalledOnValidThread());
  return false;
}

double SkypeVideoWebMediaPlayer::Duration() const {
  DCHECK(thread_checker_.CalledOnValidThread());
  return 0.0;
}

double SkypeVideoWebMediaPlayer::CurrentTime() const {
  DCHECK(thread_checker_.CalledOnValidThread());
  return 0.0;
}

bool SkypeVideoWebMediaPlayer::IsEnded() const {
  DCHECK(thread_checker_.CalledOnValidThread());
  return false;
}

blink::WebMediaPlayer::NetworkState SkypeVideoWebMediaPlayer::GetNetworkState()
    const {
  DCHECK(thread_checker_.CalledOnValidThread());
  return network_state_;
}

blink::WebMediaPlayer::ReadyState SkypeVideoWebMediaPlayer::GetReadyState()
    const {
  DCHECK(thread_checker_.CalledOnValidThread());
  return ready_state_;
}

blink::WebMediaPlayer::SurfaceLayerMode
SkypeVideoWebMediaPlayer::GetVideoSurfaceLayerMode() const {
  DCHECK(thread_checker_.CalledOnValidThread());
  return SurfaceLayerMode::kNever;
}

blink::WebString SkypeVideoWebMediaPlayer::GetErrorMessage() const {
  DCHECK(thread_checker_.CalledOnValidThread());
  return blink::WebString();
}

bool SkypeVideoWebMediaPlayer::DidLoadingProgress() {
  DCHECK(thread_checker_.CalledOnValidThread());
  return false;
}

bool SkypeVideoWebMediaPlayer::WouldTaintOrigin() const {
  DCHECK(thread_checker_.CalledOnValidThread());
  return false;
}

double SkypeVideoWebMediaPlayer::MediaTimeForTimeValue(double timeValue) const {
  DCHECK(thread_checker_.CalledOnValidThread());
  return timeValue;
}

unsigned SkypeVideoWebMediaPlayer::DecodedFrameCount() const {
  DCHECK(thread_checker_.CalledOnValidThread());
  return decoded_frame_count_;
}

unsigned SkypeVideoWebMediaPlayer::DroppedFrameCount() const {
  DCHECK(thread_checker_.CalledOnValidThread());
  return dropped_frame_count_;
}

uint64_t SkypeVideoWebMediaPlayer::AudioDecodedByteCount() const {
  DCHECK(thread_checker_.CalledOnValidThread());
  return 0;
}

uint64_t SkypeVideoWebMediaPlayer::VideoDecodedByteCount() const {
  DCHECK(thread_checker_.CalledOnValidThread());
  return 0;
}

bool SkypeVideoWebMediaPlayer::HasAvailableVideoFrame() const {
  DCHECK(thread_checker_.CalledOnValidThread());
  return received_first_frame_;
}

void SkypeVideoWebMediaPlayer::Paint(cc::PaintCanvas*,
                                     const blink::WebRect&,
                                     cc::PaintFlags&,
                                     int already_uploaded_id,
                                     VideoFrameUploadMetadata* out_metadata) {
  DCHECK(thread_checker_.CalledOnValidThread());
  // not implemented
}

blink::SkypeMediaPlayer* SkypeVideoWebMediaPlayer::GetSkypeMediaPlayer() {
  return this;
}

base::WeakPtr<blink::WebMediaPlayer> SkypeVideoWebMediaPlayer::AsWeakPtr() {
  return weak_factory_.GetWeakPtr();
}

bool SkypeVideoWebMediaPlayer::IsInitialized() const {
  DCHECK(thread_checker_.CalledOnValidThread());

  return frame_sink_->IsValid();
}

bool SkypeVideoWebMediaPlayer::IsBackgroundRendering() const {
  DCHECK(thread_checker_.CalledOnValidThread());

  return background_rendering_;
}

bool SkypeVideoWebMediaPlayer::IsTextureSharingSupported() const {
  DCHECK(thread_checker_.CalledOnValidThread());

  return context_provider_ &&
         context_provider_->ContextCapabilities().texture_sharing_supported;
}

double SkypeVideoWebMediaPlayer::VideoPositionX() const {
  DCHECK(thread_checker_.CalledOnValidThread());

  auto value = mirror_ ? crop_info_.rightOffset : crop_info_.leftOffset;
  auto total = crop_info_.leftOffset + crop_info_.rightOffset;

  if (total > 0) {
    return static_cast<double>(value) / static_cast<double>(total);
  } else {
    return 0.5;
  }
}

double SkypeVideoWebMediaPlayer::VideoPositionY() const {
  DCHECK(thread_checker_.CalledOnValidThread());

  auto value = crop_info_.topOffset;
  auto total = crop_info_.topOffset + crop_info_.bottomOffset;

  if (total > 0) {
    return static_cast<double>(value) / static_cast<double>(total);
  } else {
    return 0.5;
  }
}

gfx::Size SkypeVideoWebMediaPlayer::VideoSize() const {
  return video_size_;
}

gfx::Size SkypeVideoWebMediaPlayer::RendererSize() const {
  return renderer_size_;
}

static media::VideoPixelFormat GetVideoPixelFormat(uint32_t format) {
  switch (format) {
    case FOURCC_BGRA:
      return media::PIXEL_FORMAT_ARGB;
    case FOURCC_NV12:
      return media::PIXEL_FORMAT_NV12;
    case FOURCC_I420:
      return media::PIXEL_FORMAT_I420;
    default:
      return media::PIXEL_FORMAT_UNKNOWN;
  }
}

static gfx::Rect GetVisibleRect(unsigned int width,
                                unsigned int height,
                                const CropInfo& padding) {
  return gfx::Rect(padding.leftOffset, padding.topOffset,
                   width - (padding.leftOffset + padding.rightOffset),
                   height - (padding.topOffset + padding.bottomOffset));
}

base::TimeDelta SkypeVideoWebMediaPlayer::GetTimestamp() const {
  return base::TimeDelta::FromMicroseconds(fake_timestamp_++);
}

scoped_refptr<media::VideoFrame> SkypeVideoWebMediaPlayer::CreateVideoFrame(
    const FrameSinkClient::Frame& frame) {
  auto video_pixel_format = GetVideoPixelFormat(frame.info.format);

  if (video_pixel_format == media::PIXEL_FORMAT_UNKNOWN) {
    LogEvent(LogLevel::kLogLevelError,
             "SkypeVideoWebMediaPlayer::CreateVideoFrame - invalid format");
    return nullptr;
  }

  auto coded_size = gfx::Size(frame.info.width, frame.info.height);
  auto visible_rect =
      GetVisibleRect(frame.info.width, frame.info.height, frame.info.padding);
  auto natural_size = gfx::Size(frame.info.origWidth, frame.info.origHeight);

  if (frame.handle && texture_sharing_supported_) {
    auto video_frame = media::VideoFrame::WrapTextureHandle(
        video_pixel_format, coded_size, visible_rect, natural_size,
        frame.handle, GetTimestamp());
    if (!video_frame) {
      LogEvent(LogLevel::kLogLevelError,
               "SkypeVideoWebMediaPlayer::CreateVideoFrame - "
               "media::VideoFrame::WrapTextureHandle failed");
      return nullptr;
    }

    video_frame->mirror = frame.info.mirror;
    video_frame->buffer_name = frame.buffer_name;

    return video_frame;
  }

  if (!frame.buffer) {
    LogEvent(LogLevel::kLogLevelError,
             "SkypeVideoWebMediaPlayer::CreateVideoFrame - buffer is null");
    return nullptr;
  }

  auto video_frame = media::VideoFrame::WrapExternalData(
      video_pixel_format, coded_size, visible_rect, natural_size,
      static_cast<uint8_t*>(frame.buffer->memory()),
      frame.buffer->mapped_size(),
      base::TimeDelta::FromMicroseconds(fake_timestamp_++));

  if (!video_frame) {
    LogEvent(LogLevel::kLogLevelError,
             "SkypeVideoWebMediaPlayer::CreateVideoFrame - "
             "media::VideoFrame::WrapExternalData failed");
    return nullptr;
  }

  video_frame->mirror = frame.info.mirror;
  video_frame->buffer = frame.buffer;

  if (UseBufferSharing()) {
    video_frame->buffer_name = frame.buffer_name;
  }

  return video_frame;
}

bool SkypeVideoWebMediaPlayer::UseBufferSharing() const {
  if (auto* client = client_->GetSkypeMediaPlayerClient()) {
    return client->UseBufferSharing();
  } else {
    return false;
  }
}

void SkypeVideoWebMediaPlayer::UpdateBackgroundRendering(
    bool background_rendering) {
  if (background_rendering_ != background_rendering) {
    LogEvent(LogLevel::kLogLevelDebug,
             base::StringPrintf(
                 "SkypeVideoWebMediaPlayer::UpdateBackgroundRendering - %s",
                 background_rendering ? "true" : "false"));
    background_rendering_ = background_rendering;
    if (auto* client = client_->GetSkypeMediaPlayerClient()) {
      main_task_runner_->PostTask(
          FROM_HERE,
          base::Bind(&blink::SkypeMediaPlayerClient::BackgroundRenderingChanged,
                     base::Unretained(client)));
    }
  }
}

void SkypeVideoWebMediaPlayer::UpdateRendererSize(
    const gfx::Size& renderer_size) {
  if (renderer_size_ != renderer_size) {
    renderer_size_ = renderer_size;
    if (auto* client = client_->GetSkypeMediaPlayerClient()) {
      main_task_runner_->PostTask(
          FROM_HERE,
          base::Bind(&blink::SkypeMediaPlayerClient::RendererSizeChanged,
                     base::Unretained(client)));
    }
  }
}

void SkypeVideoWebMediaPlayer::UpdateVideoSize(const gfx::Size& video_size) {
  if (video_size_ != video_size) {
    LogEvent(LogLevel::kLogLevelDebug,
             base::StringPrintf("SkypeVideoWebMediaPlayer::UpdateVideoSize - "
                                "%d x %d -> %d x %d",
                                video_size_.width(), video_size_.height(),
                                video_size.width(), video_size.height()));
    video_size_ = video_size;
    video_size_fake_ = video_size;
    main_task_runner_->PostTask(
        FROM_HERE, base::Bind(&blink::WebMediaPlayerClient::SizeChanged,
                              base::Unretained(client_)));
  }
}

static bool IsCropInfoEqual(const CropInfo& a, const CropInfo& b) {
  return a.leftOffset == b.leftOffset && a.rightOffset == b.rightOffset &&
         a.topOffset == b.topOffset && a.bottomOffset == b.bottomOffset;
}

void SkypeVideoWebMediaPlayer::UpdateCropInfo(const CropInfo& crop_info,
                                              bool mirror) {
  if (!IsCropInfoEqual(crop_info_, crop_info) || mirror_ != mirror) {
    crop_info_ = crop_info;
    mirror_ = mirror;
    if (auto* client = client_->GetSkypeMediaPlayerClient()) {
      main_task_runner_->PostTask(
          FROM_HERE,
          base::Bind(&blink::SkypeMediaPlayerClient::VideoPositionChanged,
                     base::Unretained(client)));
    }
  }
}

void SkypeVideoWebMediaPlayer::UpdateTransparency(bool transparent) {
  DCHECK(video_layer_);

  if (transparent_ != transparent) {
    LogEvent(
        LogLevel::kLogLevelDebug,
        base::StringPrintf("SkypeVideoWebMediaPlayer::UpdateTransparency - %s",
                           transparent ? "true" : "false"));

    transparent_ = transparent;
    SetContentsOpaque(!transparent);
  }
}

void SkypeVideoWebMediaPlayer::SetContentsOpaque(bool opaque) {
  if (main_task_runner_->BelongsToCurrentThread()) {
    video_layer_->SetContentsOpaque(opaque);
  } else {
    main_task_runner_->PostTask(
        FROM_HERE, base::Bind(&SkypeVideoWebMediaPlayer::SetContentsOpaque,
                              weak_factory_.GetWeakPtr(), opaque));
  }
}

void SkypeVideoWebMediaPlayer::CreateVideoLayer(const gfx::Size& video_size,
                                                bool transparent) {
  auto video_layer =
      cc::VideoLayer::Create(compositor_, media::VIDEO_ROTATION_0);
  client_->SetCcLayer(video_layer.get());
  video_layer_ = std::move(video_layer);

  video_size_fake_ = video_size;
  client_->SizeChanged();

  create_video_layer_pending_.clear();

  LogEvent(LogLevel::kLogLevelDebug,
           base::StringPrintf(
               "SkypeVideoWebMediaPlayer::CreateVideoLayer - %d x %d, %s",
               video_size.width(), video_size.height(),
               transparent ? "true" : "false"));

  UpdateTransparency(transparent);
}

bool SkypeVideoWebMediaPlayer::CheckAndInitVideoLayer(
    const gfx::Size& video_size,
    bool transparent) {
  if (!video_layer_) {
    if (create_video_layer_pending_.test_and_set()) {
      return false;
    }

    main_task_runner_->PostTask(
        FROM_HERE,
        base::Bind(&SkypeVideoWebMediaPlayer::CreateVideoLayer,
                   weak_factory_.GetWeakPtr(), video_size, transparent));

    return false;
  }

  UpdateTransparency(transparent);

  return true;
}

void SkypeVideoWebMediaPlayer::LogEventOnMainThread(
    blink::SkypeMediaPlayerClient::LogLevel level,
    const std::string& message) {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (auto* client = client_->GetSkypeMediaPlayerClient()) {
    client->LogEvent(level, message);
  }
}

void SkypeVideoWebMediaPlayer::LogEvent(
    blink::SkypeMediaPlayerClient::LogLevel level,
    const std::string& message) {
  if (main_task_runner_->BelongsToCurrentThread()) {
    LogEventOnMainThread(level, message);
  } else {
    main_task_runner_->PostTask(
        FROM_HERE, base::Bind(&SkypeVideoWebMediaPlayer::LogEventOnMainThread,
                              weak_factory_.GetWeakPtr(), level, message));
  }
}

void SkypeVideoWebMediaPlayer::SetNetworkState(
    blink::WebMediaPlayer::NetworkState state) {
  DCHECK(thread_checker_.CalledOnValidThread());
  network_state_ = state;
  // Always notify to ensure client has the latest value.
  client_->NetworkStateChanged();
}

void SkypeVideoWebMediaPlayer::SetReadyState(
    blink::WebMediaPlayer::ReadyState state) {
  DCHECK(thread_checker_.CalledOnValidThread());
  ready_state_ = state;
  // Always notify to ensure client has the latest value.
  client_->ReadyStateChanged();
}

void SkypeVideoWebMediaPlayer::OnFirstFrameReceived() {
  DCHECK(thread_checker_.CalledOnValidThread());
  SetReadyState(blink::WebMediaPlayer::kReadyStateHaveMetadata);
  SetReadyState(blink::WebMediaPlayer::kReadyStateHaveEnoughData);

  LogEvent(LogLevel::kLogLevelDebug,
           "SkypeVideoWebMediaPlayer::OnFirstFrameReceived");
}

static scoped_refptr<media::VideoFrame> CreateBlackFrameSafe(
    const gfx::Size& size) {
  return !size.IsEmpty() ? media::VideoFrame::CreateBlackFrame(size) : nullptr;
}

scoped_refptr<media::VideoFrame> SkypeVideoWebMediaPlayer::Render(
    base::TimeTicks deadline_min,
    base::TimeTicks deadline_max,
    bool background_rendering) {
  UpdateBackgroundRendering(background_rendering);
  auto updateRenderSizeAndCropInfo = [this]() {
    UpdateRendererSize(video_layer_->bounds());
    UpdateCropInfo(current_frame_->info.cropInfo, current_frame_->info.mirror);
  };

  if (!frame_sink_->SwapRead(background_rendering)) {
    if (video_layer_ && current_frame_) {
      updateRenderSizeAndCropInfo();
      auto video_size = gfx::Size(current_frame_->info.origWidth,
                                  current_frame_->info.origHeight);
      if (!video_size.IsEmpty()) {
        UpdateVideoSize(video_size);
      }
    }
    return video_frame_;
  }

  current_frame_ =
      std::make_unique<FrameSinkClient::Frame>(frame_sink_->GetReadFrame());

  if (!received_first_frame_) {
    received_first_frame_ = true;
    main_task_runner_->PostTask(
        FROM_HERE, base::Bind(&SkypeVideoWebMediaPlayer::OnFirstFrameReceived,
                              weak_factory_.GetWeakPtr()));
  }

  auto video_size = gfx::Size(current_frame_->info.origWidth,
                              current_frame_->info.origHeight);
  bool transparent = current_frame_->info.format == FOURCC_BGRA;

  if (!video_size.IsEmpty()) {
    video_frame_ = CreateVideoFrame(*current_frame_);
    ++decoded_frame_count_;
  }

  if (!CheckAndInitVideoLayer(video_size, transparent)) {
    return video_frame_;
  }

  updateRenderSizeAndCropInfo();

  if (video_size.IsEmpty()) {
    return video_frame_ = CreateBlackFrameSafe(video_size_);
  }

  UpdateVideoSize(video_size);

  return video_frame_;
}

void SkypeVideoWebMediaPlayer::OnFrameDropped() {
  ++dropped_frame_count_;
  frame_sink_->OnFrameDropped();
}

base::TimeDelta SkypeVideoWebMediaPlayer::GetPreferredRenderInterval() {
  return viz::BeginFrameArgs::MinInterval();
}

void SkypeVideoWebMediaPlayer::OnFrameSinkClientLog(
    blink::SkypeMediaPlayerClient::LogLevel level,
    const std::string& message) {
  LogEvent(level, message);
}

void SkypeVideoWebMediaPlayer::SetContextProvider(
    scoped_refptr<viz::ContextProvider> context_provider) {
  if (context_provider_) {
    context_provider_->RemoveObserver(this);
  }

  context_provider_ = context_provider;

  if (context_provider_) {
    context_provider_->AddObserver(this);
  }
}

void SkypeVideoWebMediaPlayer::OnContextLost() {
  SetContextProvider(
      content::RenderThreadImpl::current()->SharedMainThreadContextProvider());

  bool texture_sharing_supported = IsTextureSharingSupported();
  if (texture_sharing_supported_ != texture_sharing_supported) {
    texture_sharing_supported_ = texture_sharing_supported;

    if (auto* client = client_->GetSkypeMediaPlayerClient()) {
      main_task_runner_->PostTask(
          FROM_HERE,
          base::Bind(
              &blink::SkypeMediaPlayerClient::TextureSharingSupportedChanged,
              base::Unretained(client)));
    }
  }
}

}  // namespace microsoft
