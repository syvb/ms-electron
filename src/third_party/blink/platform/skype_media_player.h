// Copyright (c) 2019 Microsoft Corporation.

#ifndef SRC_THIRD_PARTY_BLINK_PLATFORM_SKYPE_MEDIA_PLAYER_H_
#define SRC_THIRD_PARTY_BLINK_PLATFORM_SKYPE_MEDIA_PLAYER_H_

#include "ui/gfx/geometry/size.h"

namespace blink {

class SkypeMediaPlayer {
 public:
  virtual bool IsInitialized() const = 0;
  virtual bool IsBackgroundRendering() const = 0;
  virtual bool IsTextureSharingSupported() const = 0;

  virtual double VideoPositionX() const = 0;
  virtual double VideoPositionY() const = 0;

  virtual gfx::Size VideoSize() const = 0;
  virtual gfx::Size RendererSize() const = 0;
};

}  // namespace blink

#endif  // SRC_THIRD_PARTY_BLINK_PLATFORM_SKYPE_MEDIA_PLAYER_H_
