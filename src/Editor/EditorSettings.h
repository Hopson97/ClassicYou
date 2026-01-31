#pragma once

#include <fstream>

#include <nlohmann/json.hpp>

struct EditorSettings
{
    bool show_grid = true;
    bool show_2d_view = true;
    bool always_center_2d_to_3d_view = true;

    bool show_history = false;

    bool show_textures_in_2d_view = true;
    float texture_mix = 0.75;

    bool jump_to_selection_floor = true;

    bool show_messages_log = true;

    bool render_as_wireframe = false;
    bool render_vertex_normals = false;
    bool render_main_light = false;
    bool show_level_settings = false;

    bool always_show_3d_gizmos = false;

    void save() const
    {
        nlohmann::json output = {
            {"show_grid", show_grid},
            {"show_2d_view", show_2d_view},
            {"show_grid", show_grid},
            {"always_center_2d_to_3d_view", always_center_2d_to_3d_view},
            {"show_history", show_history},
            {"show_textures_in_2d_view", show_textures_in_2d_view},
            {"texture_mix", texture_mix},
            {"jump_to_selection_floor", jump_to_selection_floor},
            {"show_messages_log", show_messages_log},
            {"render_as_wireframe", render_as_wireframe},
            {"render_vertex_normals", render_vertex_normals},
            {"render_main_light", render_main_light},
            {"show_level_settings", show_level_settings},
            {"always_show_3d_gizmos", always_show_3d_gizmos},
        };

        std::ofstream settings_file("settings.json");
        settings_file << output;
    }

    void load()
    {
        std::ifstream settings_file("settings.json");
        if (settings_file)
        {
            nlohmann::json input;
            settings_file >> input;

            // clang-format off
            show_grid                       = input.value("show_grid", show_grid);
            show_2d_view                    = input.value("show_2d_view", show_2d_view);
            always_center_2d_to_3d_view     = input.value("always_center_2d_to_3d_view", always_center_2d_to_3d_view);
            show_history                    = input.value("show_history", show_history);
            show_textures_in_2d_view        = input.value("show_textures_in_2d_view", show_textures_in_2d_view);
            texture_mix                     = input.value("texture_mix", texture_mix);
            jump_to_selection_floor         = input.value("jump_to_selection_floor", jump_to_selection_floor);
            show_messages_log               = input.value("show_messages_log", show_messages_log);
            render_as_wireframe             = input.value("render_as_wireframe", render_as_wireframe);
            render_vertex_normals           = input.value("render_vertex_normals", render_vertex_normals);
            render_main_light               = input.value("render_main_light", render_main_light);
            show_level_settings             = input.value("show_level_settings", show_level_settings);
            always_show_3d_gizmos             = input.value("show_level_settings", always_show_3d_gizmos);
            // clang-format on
        }
    }
    void set_to_default()
    {
        *this = EditorSettings{};
    }
};
