#include "Model.h"

#include <print>

#include <assimp/Importer.hpp>

namespace
{
    auto texture_type_to_string(aiTextureType texture_type)
    {
        switch (texture_type)
        {

            case aiTextureType_DIFFUSE:
                return "diffuse";

            case aiTextureType_SPECULAR:
                return "specular";

                // case aiTextureType_HEIGHT:
                //     return "normal";

            default:
                return "Unknown";
        }
    }

    // From learn OpenGL
    // https://github.com/JoeyDeVries/LearnOpenGL/blob/master/includes/learnopengl/assimp_glm_helpers.h
    auto to_glm(const aiVector3d& vector)
    {
        return glm::vec3{vector.x, vector.y, vector.z};
    }

    auto to_glm(const aiMatrix4x4& from)
    {
        // clang-format off
		glm::mat4 to;
		//the a,b,c,d in assimp is the row ; the 1,2,3,4 is the column
		to[0][0] = from.a1; to[1][0] = from.a2; to[2][0] = from.a3; to[3][0] = from.a4;
		to[0][1] = from.b1; to[1][1] = from.b2; to[2][1] = from.b3; to[3][1] = from.b4;
		to[0][2] = from.c1; to[1][2] = from.c2; to[2][2] = from.c3; to[3][2] = from.c4;
		to[0][3] = from.d1; to[1][3] = from.d2; to[2][3] = from.d3; to[3][3] = from.d4;
        // clang-format on
        return to;
    }
} // namespace

Model::Model(std::vector<ModelTexture>&& textures, std::vector<ModelMesh>&& meshes)
    : textures_(std::move(textures))
    , meshes_(std::move(meshes))
{
    // Clear the meshes as they are now longer needed after buffering
    for (auto& mesh : meshes_)
    {
        mesh.mesh.buffer();

        mesh.mesh.vertices.clear();
        mesh.mesh.indices.clear();
        mesh.mesh.vertices.shrink_to_fit();
        mesh.mesh.indices.shrink_to_fit();
    }
}

void Model::draw(gl::Shader& shader)
{
    // A model can have multiple meshes, so they need to be drawn one by one.
    for (auto& mesh : meshes_)
    {
        // When setting the uniform names, we need to keep track of the index
        // For example, if we have 2 diffuse textures, the uniform names will be:
        // material.diffuse0
        // material.diffuse1
        GLuint diffuse_id = 0;
        GLuint specular_id = 0;
        GLuint normal_id = 0;

        // Each mesh can have multiple textures, so we need to bind them all to their respective
        // texture units.
        for (int i = 0; i < mesh.textures.size(); i++)
        {
            std::string n;
            // std::string& name = textures_[mesh.textures[i]].type_string;

            auto type = textures_[mesh.textures[i]].type;

            // if (type == aiTextureType_HEIGHT)
            //     n = std::to_string(diffuse_id++);
            // else if (type == aiTextureType_SPECULAR)
            //    n = std::to_string(specular_id++);
            // else if (type == aiTextureType_HEIGHT)
            //    std::print("binding normal map yo");
            // n = std::to_string(normal_id++);

            /// auto uniform = "material." + name + n;
            // shader.set_uniform(uni, i);

            // The texture unit is the same as the the index of the texture in the textures_ vector.
            textures_[mesh.textures[i]].texture.bind(i);
        }

        // Finally, we can draw the mesh.
        mesh.mesh.bind().draw_elements();
    }
}

std::optional<Model> ModelLoader::load_model(const std::filesystem::path& path)
{
    Assimp::Importer importer;

    // Load the model file using Assimp
    auto scene = importer.ReadFile(path.string(), aiProcess_Triangulate | aiProcess_FlipUVs |
                                                      aiProcess_GenNormals);

    if (!scene || !scene->mRootNode || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE)
    {

        std::println(std::cerr, "Failed to load model {}. Error: {}", path.string(),
                     importer.GetErrorString());
        return {};
    }

    directory_ = path.parent_path();

    // Process the root node of the "scene" aka the model file
    process_node(scene->mRootNode, scene);

    // Generate a list of model textures from the cached textures.
    std::vector<Model::ModelTexture> textures;
    for (auto& cached_texture : texture_cache_)
    {
        // std::println("Adding texture {} with type {} and id {}", cached_texture.path,
        //              (int)cached_texture.type, cached_texture.texture.id);

        textures.push_back({
            .texture = std::move(cached_texture.texture),
            .type_string = texture_type_to_string(cached_texture.type),
            .type = cached_texture.type,
        });
    }

    return Model{std::move(textures), std::move(meshes_)};
}

void ModelLoader::process_node(aiNode* node, const aiScene* scene)
{
    for (unsigned i = 0; i < node->mNumMeshes; i++)
    {
        auto mesh = scene->mMeshes[node->mMeshes[i]];
        meshes_.push_back(process_mesh(mesh, scene));
    }

    for (unsigned i = 0; i < node->mNumChildren; i++)
    {
        process_node(node->mChildren[i], scene);
    }
}

Model::ModelMesh ModelLoader::process_mesh(aiMesh* ai_mesh, const aiScene* scene)
{
    Model::ModelMesh mesh;

    // Load the materials from this mesh
    if (ai_mesh->mMaterialIndex >= 0)
    {
        auto material = scene->mMaterials[ai_mesh->mMaterialIndex];
        auto diffuse_maps = load_material(material, aiTextureType_DIFFUSE);
        auto specular_maps = load_material(material, aiTextureType_SPECULAR);
        // auto normal_maps = load_material(material, aiTextureType_HEIGHT);

        mesh.textures.insert(mesh.textures.end(), diffuse_maps.begin(), diffuse_maps.end());
        mesh.textures.insert(mesh.textures.end(), specular_maps.begin(), specular_maps.end());
        // mesh.textures.insert(mesh.textures.end(), normal_maps.begin(), normal_maps.end());
    }

    // Process the vertex attributes of the mesh
    mesh.mesh.vertices.reserve(ai_mesh->mNumVertices);
    for (unsigned i = 0; i < ai_mesh->mNumVertices; i++)
    {
        Vertex vertex;
        vertex.position = to_glm(ai_mesh->mVertices[i]);
        if (ai_mesh->HasNormals())
        {
            vertex.normal = to_glm(ai_mesh->mNormals[i]);
        }

        if (ai_mesh->mTextureCoords[0])
        {
            vertex.texture_coord.x = ai_mesh->mTextureCoords[0][i].x;
            vertex.texture_coord.y = ai_mesh->mTextureCoords[0][i].y;
        }

        mesh.mesh.vertices.push_back(vertex);
    }

    // Process the mesh indices
    mesh.mesh.indices.reserve(ai_mesh->mNumFaces * 3);
    for (unsigned i = 0; i < ai_mesh->mNumFaces; i++)
    {
        auto face = ai_mesh->mFaces[i];
        for (unsigned j = 0; j < face.mNumIndices; j++)
        {
            mesh.mesh.indices.push_back(face.mIndices[j]);
        }
    }
    mesh.mesh.indices.shrink_to_fit();

    return mesh;
}

std::vector<size_t> ModelLoader::load_material(aiMaterial* material, aiTextureType texture_type)
{
    // When the mesh needs to bind the texture, we need to know the index within the texture cache.
    std::vector<size_t> texture_indices;

    // Each material can have multiple textures of the same type, so we need to load all of them.
    for (unsigned i = 0; i < material->GetTextureCount(texture_type); i++)
    {
        aiString str;
        material->GetTexture(texture_type, i, &str);
        auto texture_file_name = str.C_Str();

        // Check if the texture is cached to avoid loading it multiple times
        auto index = find_texture_in_cache(texture_file_name);
        if (index != -1)
        {
            // std::println(std::cout, "Using cached texture for '{}'.", texture_file_name);
            texture_indices.push_back(index);
        }
        else
        {
            // Load the texture if it is not cached
            CachedTexture cache_texture{
                .path = texture_file_name,
                .type = texture_type,
            };

            // Load the texture and check if it was successful
            auto path = directory_ / texture_file_name;
            if (cache_texture.texture.load_from_file(
                    path, texture_type == aiTextureType_DIFFUSE ? 6 : 6, false, false))
            {
                // std::println(std::cout, "Loaded texture from path '{}'.", path.string());

                // Keep track of the texture in the cache
                texture_indices.push_back(texture_cache_.size());

                // Add the texture to the cache in case it is used again
                texture_cache_.push_back(std::move(cache_texture));
            }
            else
            {
                std::println(std::cerr, "Failed to load texture from path '{}'.", path.string());
            }
        }
    }

    return texture_indices;
}

int ModelLoader::find_texture_in_cache(std::string_view path)
{
    for (int i = 0; i < texture_cache_.size(); i++)
    {
        if (texture_cache_[i].path == path)
        {
            return i;
        }
    }
    return -1;
}
