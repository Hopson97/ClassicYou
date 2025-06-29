#pragma once

#include <filesystem>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <glad/glad.h>
#include <glm/glm.hpp>

namespace gl
{
    enum class ShaderType
    {
        Vertex = GL_VERTEX_SHADER,
        Fragment = GL_FRAGMENT_SHADER,
        Geometry = GL_GEOMETRY_SHADER,
        TesselationEvaluation = GL_TESS_EVALUATION_SHADER,
        TesselationControl = GL_TESS_CONTROL_SHADER,
        Compute = GL_COMPUTE_SHADER,
    };

    using ReplaceWord = std::pair<std::string, std::string>;

    /**
     * @brief Utility to add #include support to GLSL
     */
    class ShaderBuilder
    {
      public:
        std::optional<std::string> load_file(const std::filesystem::path& file_path, int depth = 0);

      private:
        std::unordered_map<std::filesystem::path, std::string> cache_;
    };

    class Shader
    {
      public:
        Shader() = default;
        Shader(Shader&& other) noexcept = delete;
        Shader(const Shader& other) = delete;
        Shader& operator=(Shader&& other) noexcept = delete;
        Shader& operator=(const Shader& other) = delete;
        ~Shader();

         /**
         * @brief Add a word that should be replaced within the shaders when this is used.
         *
         * @param file_path The path to the shader file.
         * @param shader_type The type of shader to load (vertex, fragment, etc.).
         * @return true if the shader stage was loaded and compiled successfully.
         * @return false if there was an error loading or compiling the shader.
         */
        void add_replace_word(ReplaceWord&& word);

        /**
         * @brief Load a shader stage from a file and compile it.
         *
         * @param file_path The path to the shader file.
         * @param shader_type The type of shader to load (vertex, fragment, etc.).
         * @return true if the shader stage was loaded and compiled successfully.
         * @return false if there was an error loading or compiling the shader.
         */
        [[nodiscard]] bool load_stage(const std::filesystem::path& file_path,
                                      ShaderType shader_type);

        /**
         * @brief The function links all loaded shader stages into a single shader program.
         * This function must be called after all shader stages have been loaded.
         *
         * @return true if the shader stages were linked successfully.
         * @return false if there was an error linking the shader stages.
         */
        [[nodiscard]] bool link_shaders();

        void bind() const;

        void set_uniform(const std::string& name, int value);
        void set_uniform(const std::string& name, float value);
        void set_uniform(const std::string& name, const glm::vec2& vector);
        void set_uniform(const std::string& name, const glm::vec3& vector);
        void set_uniform(const std::string& name, const glm::vec4& vector);
        void set_uniform(const std::string& name, const glm::mat4& matrix);

        /**
         * @brief Bind a uniform block to a specific index in the shader program.
         *
         * Example usage:
         *
         *  gl::Shader shader;
         *  // ... load shader stages and link them...
         *  int index = 1;
         *  gl::BufferObject object;
         *  object.create_as_ubo<Type>(index);
         *  shader.bind_uniform_block_index("SomeUniformBufferSTD140", index);
         *
         * @param name The name of the uniform block to bind.
         * @param index The index to bind the uniform block to.
         */
        void bind_uniform_block_index(const std::string& name, GLuint index);

      private:
        GLint get_uniform_location(const std::string& name);

      private:
        std::unordered_map<std::string, GLint> uniform_locations_;
        std::vector<GLuint> stages_;
        GLuint program_ = 0;

        std::vector<ReplaceWord> replace_words_;
    };
} // namespace gl