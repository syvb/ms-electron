// Copyright (c) 2020 Microsoft Corporation.

#include "microsoft/src/gpu/command_buffer/service/texture_manager_service.h"

#include "gpu/command_buffer/service/feature_info.h"
#include "third_party/angle/include/EGL/eglext_angle.h"
#include "ui/gl/gl_surface_egl.h"

#if defined(OS_MACOSX)
#include <IOSurface/IOSurface.h>
#include "base/mac/scoped_cftyperef.h"
#endif

namespace microsoft {

static EGLConfig GetConfig(EGLDisplay display) {
  DCHECK(display);

#if defined(OS_WIN)
  // clang-format off
  EGLint const attrib_list[] = {
      EGL_RED_SIZE, 8,
      EGL_GREEN_SIZE, 8,
      EGL_BLUE_SIZE, 8,
      EGL_ALPHA_SIZE, 8,
      EGL_BIND_TO_TEXTURE_RGBA, EGL_TRUE,
      EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
      EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
      EGL_NONE
  };
  // clang-format on
#else
  const EGLint* attrib_list = nullptr;
#endif

  EGLConfig config = nullptr;
  EGLint num_configs = 0;
  EGLBoolean result =
      eglChooseConfig(display, attrib_list, &config, 1, &num_configs);

  DCHECK(result == EGL_TRUE);
  DCHECK(num_configs = 1);
  DCHECK(config);

  return config;
}

static EGLSurface CreatePbuffer(EGLDisplay display,
                                EGLConfig config,
                                GLuint handle,
                                GLsizei width,
                                GLsizei height) {
  DCHECK(display);
  DCHECK(config);

#if defined(OS_WIN)
  // clang-format off
  EGLint const attrib_list[] = {
      EGL_WIDTH, width,
      EGL_HEIGHT, height,
      EGL_TEXTURE_FORMAT, EGL_TEXTURE_RGBA,
      EGL_TEXTURE_TARGET, EGL_TEXTURE_2D,
      EGL_NONE,
  };
  // clang-format on

  return eglCreatePbufferFromClientBuffer(
      display, EGL_D3D_TEXTURE_2D_SHARE_HANDLE_ANGLE,
      reinterpret_cast<EGLClientBuffer>(handle), config, attrib_list);
#elif defined(OS_MACOSX)
  base::ScopedCFTypeRef<IOSurfaceRef> surface(IOSurfaceLookup(handle));
  if (!surface)
    return EGL_NO_SURFACE;

  // clang-format off
  EGLint const attrib_list[] = {
      EGL_WIDTH, width,
      EGL_HEIGHT, height,
      EGL_TEXTURE_FORMAT, EGL_TEXTURE_RGBA,
      EGL_TEXTURE_TARGET, EGL_TEXTURE_RECTANGLE_ANGLE,
      EGL_TEXTURE_TYPE_ANGLE, GL_UNSIGNED_BYTE,
      EGL_TEXTURE_INTERNAL_FORMAT_ANGLE, GL_BGRA_EXT,
      EGL_IOSURFACE_USAGE_HINT_ANGLE, EGL_IOSURFACE_READ_HINT_ANGLE,
      EGL_IOSURFACE_PLANE_ANGLE, 0,
      EGL_NONE,
  };
  // clang-format on

  return eglCreatePbufferFromClientBuffer(
      display, EGL_IOSURFACE_ANGLE,
      reinterpret_cast<EGLClientBuffer>(surface.get()), config, attrib_list);
#else
  return EGL_NO_SURFACE;
#endif
}

TextureManagerService::Texture::Texture() {
  glGenTextures(1, &id_);
}

TextureManagerService::Texture::~Texture() {
  if (!context_lost_)
    glDeleteTextures(1, &id_);
}

TextureManagerService::TextureEGL::TextureEGL(EGLDisplay display,
                                              EGLConfig config)
    : Texture(), display_(display), config_(config), surface_() {}

bool TextureManagerService::TextureEGL::Bind(uint32_t handle,
                                             GLsizei width,
                                             GLsizei height) {
  surface_ = CreatePbuffer(display_, config_, handle, width, height);
  if (!surface_)
    return false;

#if defined(OS_MACOSX)
  glBindTexture(GL_TEXTURE_RECTANGLE_ARB, Texture::id_);
#else
  glBindTexture(GL_TEXTURE_2D, Texture::id_);
#endif

  return !!eglBindTexImage(display_, surface_, EGL_BACK_BUFFER);
}

TextureManagerService::TextureEGL::~TextureEGL() {
  if (!Texture::context_lost_) {
#if defined(OS_MACOSX)
    glBindTexture(GL_TEXTURE_RECTANGLE_ARB, Texture::id_);
#else
    glBindTexture(GL_TEXTURE_2D, Texture::id_);
#endif
    eglReleaseTexImage(display_, surface_, EGL_BACK_BUFFER);
    eglDestroySurface(display_, surface_);
  }
}

TextureManagerService::TextureManagerService() {
  display_ = gl::GLSurfaceEGL::GetHardwareDisplay();
  config_ = GetConfig(display_);
}

TextureManagerService::~TextureManagerService() {
  // TODO(renarsl)
}

std::shared_ptr<TextureManagerService::Texture>
TextureManagerService::GetStreamTexture(const std::string& id,
                                        uint32_t handle,
                                        GLsizei width,
                                        GLsizei height) {
  std::shared_ptr<Stream> stream;
  auto it = streams_.find(id);
  if (it == streams_.end()) {
    stream = std::make_shared<Stream>();
    streams_[id] = stream;
  } else {
    stream = it->second;
  }
  if (!stream)
    return {};
  return stream->GetTexture(display_, config_, handle, width, height);
}

void TextureManagerService::ReleaseStream(const std::string& id) {
  streams_.erase(id);
}

void TextureManagerService::SetContextLost() {
  for (auto stream : streams_) {
    stream.second->SetContextLost();
  }
}

// Complex class/struct needs an explicit out-of-line constructor.
TextureManagerService::Stream::Stream() = default;

// Complex class/struct needs an explicit out-of-line destructor.
TextureManagerService::Stream::~Stream() = default;

std::shared_ptr<TextureManagerService::Texture>
TextureManagerService::Stream::GetTexture(EGLDisplay display,
                                          EGLConfig config,
                                          uint32_t handle,
                                          GLsizei width,
                                          GLsizei height) {
  std::shared_ptr<Texture> texture;

  auto it = texture_map_.find(handle);
  if (it == texture_map_.end()) {
    if (texture_map_.size() >= TARGET_SURFACE_COUNT) {
      RemoveOldestUsed();
    }
    auto texture_egl = std::make_shared<TextureEGL>(display, config);
    if (!texture_egl)
      return {};
    if (!texture_egl->Bind(handle, width, height))
      return {};
    texture = std::static_pointer_cast<Texture>(texture_egl);
    texture_map_[handle] = texture;
  } else {
    texture = it->second;
  }
  auto remove_use = std::find(use_.begin(), use_.end(), handle);
  if (remove_use != use_.end()) {
    use_.erase(remove_use);
  }
  use_.push_back(handle);

  return texture;
}

void TextureManagerService::Stream::ReleaseTexture(uint32_t handle) {
  auto it = texture_map_.find(handle);
  if (it == texture_map_.end())
    return;

  texture_map_.erase(it);
}

void TextureManagerService::Stream::RemoveOldestUsed() {
  if (!use_.empty()) {
    auto oldest = use_.front();
    use_.erase(use_.begin());
    ReleaseTexture(oldest);
  }
}

void TextureManagerService::Stream::SetContextLost() {
  for (auto texture : texture_map_) {
    texture.second->SetContextLost();
  }
}

bool IsTextureSharingSupported(const gpu::gles2::FeatureInfo* feature_info) {
  DCHECK(feature_info);
#if defined(OS_WIN)
  return feature_info->feature_flags().chromium_copy_texture &&
         gl::GLSurfaceEGL::HasEGLExtension(
             "EGL_ANGLE_d3d_share_handle_client_buffer");
#elif defined(OS_MACOSX)
  return feature_info->feature_flags().chromium_copy_texture &&
         gl::GLSurfaceEGL::HasEGLExtension("EGL_ANGLE_iosurface_client_buffer");
#else
  return false;
#endif
}

}  // namespace microsoft
