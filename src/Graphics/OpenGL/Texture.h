#pragma once

#include <filesystem>
#include <print>
#include <string_view>
#include <unordered_map>

#include <SFML/Graphics/Image.hpp>
#include <glad/glad.h>
#include <glm/glm.hpp>

namespace gl
{
    enum class TextureInternalFormat
    {
        RGB = GL_RGB,
        RGBA = GL_RGBA,
    };

    enum class TextureFormat
    {
        RGB8 = GL_RGB8,
        RGB16 = GL_RGB16,

        RGBA8 = GL_RGBA8,
        RGBA16 = GL_RGBA16,

        RGBA16F = GL_RGBA16F,
        RGBA32F = GL_RGBA32F
    };

    enum class TextureMinFilter
    {
        Nearest = GL_NEAREST,
        Linear = GL_LINEAR,
        NearestMipmapNearest = GL_NEAREST_MIPMAP_NEAREST,
        LinearMipmapNearest = GL_LINEAR_MIPMAP_NEAREST,
        NearestMipmapLinear = GL_NEAREST_MIPMAP_LINEAR,
        LinearMipmapLinear = GL_LINEAR_MIPMAP_LINEAR,
    };

    enum class TextureMagFilter
    {
        Nearest = GL_NEAREST,
        Linear = GL_LINEAR,
    };

    enum class TextureCompareFunction
    {
        LessThanOrEqual = GL_LEQUAL,
        GreaterThanOrEqual = GL_GEQUAL,
        LessThan = GL_LESS,
        GreaterThan = GL_GREATER,
        Equal = GL_EQUAL,
        NotEqual = GL_NOTEQUAL,
        Always = GL_ALWAYS,
        Never = GL_NEVER,
    };

    enum class TextureCompareMode
    {
        CompareReferenceToTexture = GL_COMPARE_REF_TO_TEXTURE,
        None = GL_NONE
    };

    enum class TextureWrap
    {
        ClampToEdge = GL_CLAMP_TO_EDGE,
        ClampToBorder = GL_CLAMP_TO_BORDER,
        MirroredRepeat = GL_MIRRORED_REPEAT,
        Repeat = GL_REPEAT,
        MirrorClampToEdge = GL_MIRROR_CLAMP_TO_EDGE,
    };

    struct TextureParameters
    {
        TextureMinFilter min_filter = TextureMinFilter::Linear;
        TextureMagFilter mag_filter = TextureMagFilter::Linear;
        TextureWrap wrap_s = TextureWrap::ClampToEdge;
        TextureWrap wrap_t = TextureWrap::ClampToEdge;
        TextureWrap wrap_r = TextureWrap::ClampToEdge;
    };

    constexpr static TextureParameters TEXTURE_PARAMS_MIPMAP = {
        .min_filter = TextureMinFilter::LinearMipmapLinear,
        .mag_filter = TextureMagFilter::Linear,
        .wrap_s = TextureWrap::Repeat,
        .wrap_t = TextureWrap::Repeat,
        .wrap_r = TextureWrap::Repeat,
    };

    constexpr static TextureParameters TEXTURE_PARAMS_NEAREST = {
        .min_filter = gl::TextureMinFilter::Nearest,
        .mag_filter = gl::TextureMagFilter::Nearest,
        .wrap_s = gl::TextureWrap::Repeat,
        .wrap_t = gl::TextureWrap::Repeat,
        .wrap_r = TextureWrap::Repeat,
    };
    // clang-format off

    struct TextureResource
    {
        GLuint id = 0;

        TextureResource(GLenum target) { glCreateTextures(target, 1, &id); }   
        virtual ~TextureResource() { if(id != 0) glDeleteTextures(1, &id); }

        TextureResource           (const TextureResource& other) = delete;  
        TextureResource& operator=(const TextureResource& other) = delete;  

        TextureResource& operator=(TextureResource&& other) noexcept { id = other.id;  other.id = 0; return *this; }   
        TextureResource (TextureResource&& other) noexcept : id  (other.id){ other.id = 0; }   

        void bind(GLuint unit) const { assert(id); glBindTextureUnit(unit, id); }
        // clang-format on

        void set_filters(const TextureParameters& filters);
        void set_min_filter(TextureMinFilter filter);
        void set_mag_filter(TextureMagFilter filter);
        void set_wrap_s(TextureWrap wrap);
        void set_wrap_t(TextureWrap wrap);
        void set_wrap_r(TextureWrap wrap);
        void set_compare_function(TextureCompareFunction function);
        void set_compare_mode(TextureCompareMode mode);
    };

    enum class Texture2DTarget
    {
        Texture2D = GL_TEXTURE_2D,
        Multisample = GL_TEXTURE_2D_MULTISAMPLE
    };

    struct Texture2D : public TextureResource
    {
        Texture2D(Texture2DTarget target = Texture2DTarget::Texture2D);

        GLuint create(GLsizei width, GLsizei height, GLsizei levels = 1,
                      TextureParameters filters = {}, TextureFormat format = TextureFormat::RGB8);

        GLuint create_multi_sample(GLsizei width, GLsizei height, GLsizei samples = 4,
                                   TextureParameters filters = {},
                                   TextureFormat format = TextureFormat::RGB8);

        GLuint create_depth_texture(GLsizei width, GLsizei height);

        [[nodiscard]] bool
        load_from_memory(const void* data, GLsizei width, GLsizei height, GLsizei levels,
                         TextureParameters filters = TEXTURE_PARAMS_MIPMAP,
                         TextureInternalFormat internal_format = TextureInternalFormat::RGBA,
                         TextureFormat format = TextureFormat::RGBA8);

        [[nodiscard]] bool load_from_image(const sf::Image& image, GLsizei levels,
                                           TextureParameters filters = TEXTURE_PARAMS_MIPMAP,
                                           TextureFormat format = TextureFormat::RGBA8);

        [[nodiscard]] bool load_from_file(const std::filesystem::path& path, GLsizei levels,
                                          bool flip_vertically, bool flip_horizontally,
                                          TextureParameters filters = TEXTURE_PARAMS_MIPMAP,
                                          TextureFormat format = TextureFormat::RGBA8);

        [[nodiscard]] bool is_loaded() const;

      private:
        bool is_loaded_ = false;
    };

    struct CubeMapTexture : public TextureResource
    {
        CubeMapTexture();

        /**
         * @brief Loads a cube map texture from a folder containing six images, one for each face of
         * the cube.
         * The images should be named as follows:
         * back.png, front.png, left.png, right.png, top.png, bottom.png
         *
         * @param folder The folder containing the six images for the cube map faces.
         * @return true if the cube map was loaded successfully.
         * @return false if there was an error loading any of the images.
         */
        [[nodiscard]] bool load_from_folder(const std::filesystem::path& folder);

        [[nodiscard]] bool is_loaded() const;

        /**
         * @brief Creates a cube map texture using GL_DEPTH_COMPONENT for each face.
         *
         * @param width Width of the cube map texture.
         * @param height Height of the cube map texture.
         * @return GLuint The OpenGL texture ID of the cube map texture.
         */
        GLuint create_depth_cube(GLsizei width, GLsizei height);

      private:
        bool is_loaded_ = false;
    };

    class Texture2DArray final : public TextureResource
    {
      public:
        Texture2DArray();

        void create(GLuint texture_size, GLuint texture_count,
                    TextureParameters filters = TEXTURE_PARAMS_MIPMAP);
        GLuint create_depth_texture(GLsizei width, GLsizei height);

        std::pair<bool, GLuint>
        add_texture_from_file(const std::filesystem::path& path, GLsizei levels,
                              bool flip_vertically, bool flip_horizontally,
                              TextureInternalFormat format = TextureInternalFormat::RGBA);

      private:
        GLuint texture_size_ = 0;
        GLuint texture_count_ = 0;
        GLuint max_textures_ = 0;
    };
} // namespace gl