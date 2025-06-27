#pragma once

#include "Mesh.h"
#include "OpenGL/Texture.h"

class Skybox
{
  public:
    bool init(const std::filesystem::path& folder);
    void render();

  private:
    gl::Shader shader_;
    gl::CubeMapTexture texture_;
    Mesh3D skybox_mesh_ = generate_centered_cube_mesh({150.0f, 150.0f, 150.0f});
};