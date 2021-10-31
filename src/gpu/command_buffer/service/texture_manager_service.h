// Copyright (c) 2020 Microsoft Corporation.

#ifndef SRC_GPU_COMMAND_BUFFER_SERVICE_TEXTURE_MANAGER_SERVICE_H_
#define SRC_GPU_COMMAND_BUFFER_SERVICE_TEXTURE_MANAGER_SERVICE_H_

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "ui/gl/gl_bindings.h"

namespace gpu {
namespace gles2 {
class FeatureInfo;
}
}  // namespace gpu

namespace microsoft {

class TextureManagerService {
 public:
  struct Texture {
    Texture();
    virtual ~Texture();

    GLuint GetId() const { return id_; }
    void SetContextLost() { context_lost_ = true; }

   protected:
    GLuint id_ = GL_NONE;
    bool context_lost_ = false;
  };

  struct Stream {
    Stream();
    ~Stream();

    std::shared_ptr<TextureManagerService::Texture> GetTexture(
        EGLDisplay display,
        EGLConfig config,
        uint32_t handle,
        GLsizei width,
        GLsizei height);
    void ReleaseTexture(uint32_t handle);
    void RemoveOldestUsed();
    void SetContextLost();

    enum {
      TARGET_SURFACE_COUNT = 3,
    };
    std::vector<uint32_t> use_;
    std::map<uint32_t, std::shared_ptr<Texture>> texture_map_;
  };

  TextureManagerService();
  ~TextureManagerService();

  std::shared_ptr<TextureManagerService::Texture> GetStreamTexture(
      const std::string& id,
      uint32_t handle,
      GLsizei width,
      GLsizei height);
  void ReleaseStream(const std::string& id);
  void SetContextLost();

 private:
  struct TextureEGL : Texture {
    TextureEGL(EGLDisplay display, EGLConfig config);
    ~TextureEGL() override;

    EGLSurface GetSurface() const { return surface_; }
    bool Bind(uint32_t handle, GLsizei width, GLsizei height);

   private:
    EGLDisplay display_;
    EGLConfig config_;
    EGLSurface surface_;
  };

  EGLDisplay display_;
  EGLConfig config_;

  std::map<std::string, std::shared_ptr<Stream>> streams_;
};

bool IsTextureSharingSupported(const gpu::gles2::FeatureInfo* feature_info);

}  // namespace microsoft

#endif  // SRC_GPU_COMMAND_BUFFER_SERVICE_TEXTURE_MANAGER_SERVICE_H_
