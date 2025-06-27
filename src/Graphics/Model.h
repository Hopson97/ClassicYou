#pragma once

#include <filesystem>

#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include "Mesh.h"
#include "OpenGL/Texture.h"

class Model
{
  public:
    struct ModelMesh
    {
        Mesh3D mesh;

        // Index to the "textures_" vector
        std::vector<std::size_t> textures;
    };

    struct ModelTexture
    {
        gl::Texture2D texture;
        std::string type_string;
        aiTextureType type;
    };

    Model() = default;
    Model(std::vector<ModelTexture>&& textures, std::vector<ModelMesh>&& meshes);

    void draw(gl::Shader& shader);

  private:
    std::vector<ModelTexture> textures_;
    std::vector<ModelMesh> meshes_;
};

class ModelLoader
{
    struct CachedTexture
    {
        gl::Texture2D texture;
        std::string path;
        aiTextureType type;
    };

  public:
    std::optional<Model> load_model(const std::filesystem::path& path);

  private:
    void process_node(aiNode* node, const aiScene* scene);
    Model::ModelMesh process_mesh(aiMesh* ai_mesh, const aiScene* scene);
    std::vector<size_t> load_material(aiMaterial* material, aiTextureType texture_type);

    int find_texture_in_cache(std::string_view path);

    std::vector<Model::ModelMesh> meshes_;
    std::vector<CachedTexture> texture_cache_;

    std::filesystem::path directory_;
};