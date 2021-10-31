// Copyright (c) 2019 Microsoft Corporation.

#ifndef SRC_MEDIA_BLINK_SKYPEVIDEO_WEBMEDIAPLAYER_H_
#define SRC_MEDIA_BLINK_SKYPEVIDEO_WEBMEDIAPLAYER_H_

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/optional.h"
#include "base/threading/thread.h"
#include "components/viz/common/gpu/context_lost_observer.h"
#include "media/blink/media_blink_export.h"
#include "media/blink/multibuffer_data_source.h"
#include "media/blink/video_frame_compositor.h"
#include "microsoft/src/media_blink/frame_sink_client.h"
#include "microsoft/src/media_blink/image_info.h"
#include "microsoft/src/third_party/blink/platform/skype_media_player.h"
#include "microsoft/src/third_party/blink/platform/skype_media_player_client.h"
#include "third_party/blink/public/platform/web_media_player.h"
#include "third_party/blink/public/platform/web_size.h"
#include "url/gurl.h"

namespace blink {
class WebMediaPlayerClient;
}

namespace base {
class SingleThreadTaskRunner;
}

namespace cc {
class VideoLayer;
}

namespace viz {
class ContextProvider;
}

namespace microsoft {

class MEDIA_BLINK_EXPORT SkypeVideoWebMediaPlayer
    : public blink::WebMediaPlayer,
      public blink::SkypeMediaPlayer,
      public media::VideoRendererSink::RenderCallback,
      public microsoft::FrameSinkClientDelegate,
      public viz::ContextLostObserver {
 public:
  SkypeVideoWebMediaPlayer(
      blink::WebMediaPlayerClient* client,
      const GURL& gurl,
      scoped_refptr<base::SingleThreadTaskRunner> compositor_task_runner);

  ~SkypeVideoWebMediaPlayer() override;

  // blink::WebMediaPlayer
  LoadTiming Load(LoadType,
                  const blink::WebMediaPlayerSource&,
                  CorsMode) override;
  void Play() override;
  void Pause() override;
  void Seek(double seconds) override;
  void SetRate(double) override;
  void SetVolume(double) override;
  void SetLatencyHint(double seconds) override;
  void SetPreservesPitch(bool preserves_pitch) override;
  void OnRequestPictureInPicture() override;
  void OnPictureInPictureAvailabilityChanged(bool available) override;
  blink::WebTimeRanges Buffered() const override;
  blink::WebTimeRanges Seekable() const override;
  void SetSinkId(const blink::WebString& sink_id,
                 blink::WebSetSinkIdCompleteCallback) override;
  bool HasVideo() const override;
  bool HasAudio() const override;
  gfx::Size NaturalSize() const override;
  gfx::Size VisibleSize() const override;
  bool Paused() const override;
  bool Seeking() const override;
  double Duration() const override;
  double CurrentTime() const override;
  bool IsEnded() const override;
  NetworkState GetNetworkState() const override;
  ReadyState GetReadyState() const override;
  SurfaceLayerMode GetVideoSurfaceLayerMode() const override;
  blink::WebString GetErrorMessage() const override;
  bool DidLoadingProgress() override;
  bool WouldTaintOrigin() const override;
  double MediaTimeForTimeValue(double timeValue) const override;
  unsigned DecodedFrameCount() const override;
  unsigned DroppedFrameCount() const override;
  uint64_t AudioDecodedByteCount() const override;
  uint64_t VideoDecodedByteCount() const override;
  bool HasAvailableVideoFrame() const override;
  void Paint(cc::PaintCanvas*,
             const blink::WebRect&,
             cc::PaintFlags&,
             int already_uploaded_id,
             VideoFrameUploadMetadata* out_metadata) override;

  blink::SkypeMediaPlayer* GetSkypeMediaPlayer() override;

  base::WeakPtr<blink::WebMediaPlayer> AsWeakPtr() override;

  // blink::SkypeMediaPlayer
  bool IsInitialized() const override;
  bool IsBackgroundRendering() const override;
  bool IsTextureSharingSupported() const override;

  double VideoPositionX() const override;
  double VideoPositionY() const override;

  gfx::Size VideoSize() const override;
  gfx::Size RendererSize() const override;

  // media::VideoRendererSink::RenderCallback
  scoped_refptr<media::VideoFrame> Render(base::TimeTicks deadline_min,
                                          base::TimeTicks deadline_max,
                                          bool background_rendering) override;
  void OnFrameDropped() override;
  base::TimeDelta GetPreferredRenderInterval() override;

  // microsoft::FrameSinkClientDelegate
  void OnFrameSinkClientLog(blink::SkypeMediaPlayerClient::LogLevel level,
                            const std::string& message) override;

  // viz::ContextLostObserver
  void OnContextLost() override;

 private:
  base::TimeDelta GetTimestamp() const;

  scoped_refptr<media::VideoFrame> CreateVideoFrame(
      const FrameSinkClient::Frame& frame);

  bool UseBufferSharing() const;

  void UpdateBackgroundRendering(bool background_rendering);
  void UpdateRendererSize(const gfx::Size& renderer_size);
  void UpdateVideoSize(const gfx::Size& video_size);
  void UpdateCropInfo(const CropInfo& crop_info, bool mirror);
  void UpdateTransparency(bool transparent);

  void SetContentsOpaque(bool opaque);

  void CreateVideoLayer(const gfx::Size& video_size, bool transparent);
  bool CheckAndInitVideoLayer(const gfx::Size& video_size, bool transparent);

  void LogEventOnMainThread(blink::SkypeMediaPlayerClient::LogLevel level,
                            const std::string& message);

  void LogEvent(blink::SkypeMediaPlayerClient::LogLevel level,
                const std::string& message);

  void SetNetworkState(blink::WebMediaPlayer::NetworkState state);
  void SetReadyState(blink::WebMediaPlayer::ReadyState state);

  void OnFirstFrameReceived();

  void SetContextProvider(scoped_refptr<viz::ContextProvider> context_provider);

  blink::WebMediaPlayerClient* client_;
  scoped_refptr<base::SingleThreadTaskRunner> compositor_task_runner_;
  scoped_refptr<base::SingleThreadTaskRunner> main_task_runner_;
  // Deleted on |compositor_task_runner_|.
  media::VideoFrameCompositor* compositor_;
  scoped_refptr<cc::VideoLayer> video_layer_;

  scoped_refptr<viz::ContextProvider> context_provider_;
  bool texture_sharing_supported_ = false;

  // Used for DCHECKs to ensure methods calls executed in the correct thread.
  base::ThreadChecker thread_checker_;

  blink::WebMediaPlayer::NetworkState network_state_ =
      blink::WebMediaPlayer::kNetworkStateEmpty;
  blink::WebMediaPlayer::ReadyState ready_state_ =
      blink::WebMediaPlayer::kReadyStateHaveNothing;

  scoped_refptr<media::VideoFrame> video_frame_;
  mutable int64_t fake_timestamp_ = 0;

  std::unique_ptr<FrameSinkClient> frame_sink_;

  bool background_rendering_ = false;
  bool received_first_frame_ = false;
  gfx::Size video_size_;
  gfx::Size video_size_fake_;
  gfx::Size renderer_size_;
  CropInfo crop_info_;
  bool mirror_ = false;
  base::Optional<bool> transparent_;
  std::atomic_flag create_video_layer_pending_ = ATOMIC_FLAG_INIT;

  std::atomic<unsigned> decoded_frame_count_ = ATOMIC_VAR_INIT(0);
  std::atomic<unsigned> dropped_frame_count_ = ATOMIC_VAR_INIT(0);
  std::unique_ptr<FrameSinkClient::Frame> current_frame_;

  base::WeakPtrFactory<SkypeVideoWebMediaPlayer> weak_factory_{this};

  DISALLOW_COPY_AND_ASSIGN(SkypeVideoWebMediaPlayer);
};

}  // namespace microsoft

#endif  // SRC_MEDIA_BLINK_SKYPEVIDEO_WEBMEDIAPLAYER_H_
