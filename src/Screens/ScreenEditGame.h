#pragma once

#include <unordered_map>

#include "../Editor/DrawingPad.h"
#include "../Editor/EditConstants.h"
#include "../Editor/Tool.h"
#include "../Graphics/Camera.h"
#include "../Graphics/CameraController.h"
#include "../Graphics/Mesh.h"
#include "../Graphics/OpenGL/BufferObject.h"
#include "../Graphics/OpenGL/Texture.h"
#include "../Settings.h"
#include "Screen.h"

class LevelTextures
{
  public:
    bool register_texture(const std::string& name, const std::filesystem::path& texture_file_path,
                          gl::Texture2DArray& textures)
    {
        if (texture_map.find(name) != texture_map.end())
        {
            return true;
        }

        // Load the texture from disk - todo maybe load the image once and use for both?
        auto [loaded, id] = textures.add_texture_from_file(texture_file_path, 4, false, false);
        if (loaded)
        {
            texture_map.emplace(name, id);

            // Also load the "TEXTURE_2D" for GUIs
            gl::Texture2D texture;
            if (!texture.load_from_file(texture_file_path, 4, false, false,
                                        gl::TEXTURE_PARAMS_NEAREST))
            {
                return false;
            }

            texture_2d_map.emplace(name, std::move(texture));
            return true;
        }

        return false;
    }

    GLfloat get_texture(const std::string& name)
    {
        auto itr = texture_map.find(name);
        if (itr == texture_map.end())
        {
            return -1.0f;
        }
        return itr->second;
    }

    // For use in rendering 3D objects, this maps a name the "layer" within a GL_TEXTURE_2D_ARRAY
    std::unordered_map<std::string, GLuint> texture_map;

    // For use in rendering ImGUI, this maps the name to a 2D texture
    std::unordered_map<std::string, gl::Texture2D> texture_2d_map;
};

class ScreenEditGame final : public Screen
{
  public:
    ScreenEditGame(ScreenManager& screens);

    bool on_init() override;
    void on_event(const sf::Event& event) override;
    void on_update(const Keyboard& keyboard, sf::Time dt) override;
    void on_fixed_update(sf::Time dt) override;
    void on_render(bool show_debug) override;

  private:
    void pause_menu();
    void debug_gui();

    void render_scene(gl::Shader& shader);

    Camera camera_;

    Mesh3D grid_mesh_ = generate_grid_mesh(WORLD_SIZE, WORLD_SIZE);

    gl::BufferObject matrices_ssbo_;
    gl::Shader scene_shader_;
    gl::Shader world_geometry_shader_;

    Mesh3D selection_mesh_ = generate_quad_mesh(0.25, 0.25);

    DrawingPad drawing_pad_;

    bool rotation_locked_ = true;
    bool game_paused_ = false;
    Settings settings_;

    struct EditorState
    {
        glm::ivec2 node_hovered;
    };
    EditorState editor_state_;

    CameraKeybinds camera_keybinds_;

    LevelTextures level_texures_;
    gl::Texture2DArray texture_;
    CreateWallTool tool_;
};