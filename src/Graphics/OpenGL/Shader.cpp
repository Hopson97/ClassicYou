#include "Shader.h"

#include <cstring>
#include <iostream>
#include <print>

#include <glm/gtc/type_ptr.hpp>

#include "../../Util/Util.h"

namespace
{
    enum class IVParameter
    {
        CompileStatus = GL_COMPILE_STATUS,
        LinkStatus = GL_LINK_STATUS
    };

    [[nodiscard]] constexpr auto to_string(IVParameter param)
    {
        switch (param)
        {
            case IVParameter::CompileStatus:
                return "compile";

            case IVParameter::LinkStatus:
                return "link";
        }
        return "ShouldNeverGetHere";
    }

    [[nodiscard]] auto get_program_info_log(GLuint shader, GLsizei length)
    {
        std::string buffer(length, ' ');
        glGetProgramInfoLog(shader, length, NULL, buffer.data());
        buffer.erase(buffer.find_last_not_of(" \t\n\r\f\v") + 1);
        return buffer;
    }

    [[nodiscard]] auto get_shader_info_log(GLuint shader, GLsizei length)
    {
        std::string buffer(length, ' ');
        glGetShaderInfoLog(shader, length, NULL, buffer.data());
        buffer.erase(buffer.find_last_not_of(" \t\n\r\f\v") + 1);
        return buffer;
    }

    template <IVParameter parameter>
    [[nodiscard]] auto verify_shader(GLuint shader)
    {
        // Verify
        GLint status = 0;
        if constexpr (parameter == IVParameter::CompileStatus)
        {
            glGetShaderiv(shader, static_cast<GLenum>(parameter), &status);
        }
        else if constexpr (parameter == IVParameter::LinkStatus)
        {
            glGetProgramiv(shader, static_cast<GLenum>(parameter), &status);
        }
        else
        {
            std::println(std::cerr, "Unkown verify type for action '{}'.", to_string(parameter));
        }

        if (status == GL_FALSE)
        {
            GLsizei length;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);

            if constexpr (parameter == IVParameter::CompileStatus)
            {
                glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
            }
            else if constexpr (parameter == IVParameter::LinkStatus)
            {
                glGetProgramiv(shader, GL_INFO_LOG_LENGTH, &length);
            }
            std::println(std::cerr, "Failed to {} shader. Error: {}", to_string(parameter),
                         get_shader_info_log(shader, length));
            return false;
        }
        return true;
    }

    [[nodiscard]] GLuint compile_shader(const char* source, GLuint shader_type)
    {
        GLuint shader = glCreateShader(shader_type);
        glShaderSource(shader, 1, (const GLchar* const*)&source, nullptr);
        glCompileShader(shader);

        if (!verify_shader<IVParameter::CompileStatus>(shader))
        {
            return 0;
        }
        return shader;
    }
} // namespace

namespace gl
{
    std::optional<std::string> ShaderBuilder::load_file(const std::filesystem::path& file_path,
                                                        int depth)
    {
        if (depth > 10)
        {
            std::println(std::cerr,
                         "Potential circular include detected with {}. Consider exiting!",
                         file_path.string());
        }

        auto file_contents = read_file_to_string(file_path);
        if (file_contents.empty())
        {
            return {};
        }

        std::istringstream stream(file_contents);

        std::string output;
        output.reserve(file_contents.length() * 2);

        std::string line;
        int line_number = 1;
        while (std::getline(stream, line))
        {
            if (line.starts_with("#include"))
            {
                auto start = line.find_first_of("\"") + 1;
                auto end = line.find_last_of("\"") + 1;
                auto filename = line.substr(start, end - start - 1);

                auto path = file_path.parent_path() / filename;

                auto content = load_file(path, depth++);
                if (content)
                {
                    output += "#line 1\n";
                    output += *content;
                    output += "#line " + std::to_string(line_number + 2) + '\n';
                }
                else
                {
                    std::println("Failed to load file {} on line {}", filename, line_number);
                    return {};
                }
            }
            else
            {
                output += line + '\n';
            }
            line_number++;
        }
        return output;
    }

    Shader::~Shader()
    {
        glDeleteProgram(program_);
    }

    void Shader::add_replace_word(ReplaceWord&& word)
    {
        replace_words_.push_back(std::move(word));
    }

    bool Shader::load_stage(const std::filesystem::path& file_path, ShaderType shader_type)
    {
        // Load the files into strings and verify
        ShaderBuilder builder;
        auto source = builder.load_file(file_path);
        if (!source)
        {
            return false;
        }

        // Replace words
        for (auto& [from, to] : replace_words_)
        {
            size_t pos = 0;
            while ((pos = source->find(from, pos)) != std::string::npos)
            {
                source->replace(pos, from.length(), to);
                pos += to.length();
            }
        }

        if (source->length() == 0)
        {
            return false;
        }

        GLuint shader = compile_shader(source->c_str(), static_cast<GLenum>(shader_type));
        if (!shader)
        {
            std::println(std::cerr, "Failed to compile file '{}'.", file_path.string());
            return false;
        }
        stages_.push_back(shader);

        return true;
    }

    bool Shader::link_shaders()
    {
        // Link the shaders together and verify the link status
        program_ = glCreateProgram();
        for (auto stage : stages_)
        {
            glAttachShader(program_, stage);
        }
        glLinkProgram(program_);

        if (!verify_shader<IVParameter::LinkStatus>(program_))
        {
            std::println(std::cerr, "Failed to link shader.");

            return false;
        }
        glValidateProgram(program_);

        int status = 0;
        glGetProgramiv(program_, GL_VALIDATE_STATUS, &status);
        if (status == GL_FALSE)
        {
            std::println(std::cerr, "Failed to validate shader program: {}.",
                         get_program_info_log(program_, 1024));
            return false;
        }

        // Delete the temporary shaders
        for (auto& shader : stages_)
        {
            glDeleteShader(shader);
        }
        stages_.clear();
        stages_.shrink_to_fit();

        replace_words_.clear();
        replace_words_.shrink_to_fit();

        return true;
    }

    void Shader::bind() const
    {
        glUseProgram(program_);
    }

    void Shader::set_uniform(const std::string& name, int value)
    {
        glProgramUniform1i(program_, get_uniform_location(name), value);
    }

    void Shader::set_uniform(const std::string& name, float value)
    {
        glProgramUniform1f(program_, get_uniform_location(name), value);
    }

    void Shader::set_uniform(const std::string& name, const glm::vec2& vector)
    {
        glProgramUniform2fv(program_, get_uniform_location(name), 1, glm::value_ptr(vector));
    }

    void Shader::set_uniform(const std::string& name, const glm::vec3& vector)
    {
        glProgramUniform3fv(program_, get_uniform_location(name), 1, glm::value_ptr(vector));
    }

    void Shader::set_uniform(const std::string& name, const glm::vec4& vector)
    {
        glProgramUniform4fv(program_, get_uniform_location(name), 1, glm::value_ptr(vector));
    }

    void Shader::set_uniform(const std::string& name, const glm::mat4& matrix)
    {
        glProgramUniformMatrix4fv(program_, get_uniform_location(name), 1, GL_FALSE,
                                  glm::value_ptr(matrix));
    }

    void Shader::bind_uniform_block_index(const std::string& name, GLuint index)
    {
        glUniformBlockBinding(program_, glGetUniformBlockIndex(program_, name.c_str()), index);
    }

    GLint Shader::get_uniform_location(const std::string& name)
    {
        auto itr = uniform_locations_.find(name);
        if (itr == uniform_locations_.end())
        {
            auto location = glGetUniformLocation(program_, name.c_str());
            if (location == -1)
            {
                std::println(std::cerr, "Cannot find uniform location {}'", name);
                return 0;
            }
            uniform_locations_.insert({name, location});

            return location;
        }

        return itr->second;
    }
} // namespace gl
