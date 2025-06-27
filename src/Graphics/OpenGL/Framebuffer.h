#pragma once

#include <memory>
#include <unordered_map>

#include "GLResource.h"
#include "Texture.h"

namespace gl
{
    enum class FramebufferTarget
    {
        Framebuffer = GL_FRAMEBUFFER,
        DrawFramebuffer = GL_DRAW_FRAMEBUFFER,
        ReadFramebuffer = GL_READ_FRAMEBUFFER,

    };

    /**
     * @brief Class representing an OpenGL framebuffer object.
     */
    class Framebuffer : public GLResource<glCreateFramebuffers, glDeleteFramebuffers>
    {
      public:
        Framebuffer(GLuint width, GLuint height);
        ~Framebuffer();

        Framebuffer& operator=(Framebuffer&& other) noexcept;
        Framebuffer(Framebuffer&& other) noexcept;

        /**
         * @brief Bind the framebuffer to the specified target.
         *
         * @param target The target to bind the framebuffer to.
         * @param clear If true, the framebuffer will be cleared before binding.
         */
        void bind(FramebufferTarget target, bool clear) const;

        /**
         * @brief Bind one of the textures attached to the framebuffer to a texture unit.
         *
         * @param index The index of the texture to bind within the framebuffer.
         * @param unit The texture unit to bind the texture to.
         */
        void bind_texture(GLuint index, GLuint unit) const;

        /**
         * @brief Get the texture id object
         *
         * @param index The index of the texture to retrieve.
         * @return GLuint The texture ID of the specified texture.
         */
        GLuint get_texture_id(GLuint index) const;

        TextureResource& get_texture(GLuint index);

        /**
         * @brief Attach a colour texture to the framebuffer.
         *
         * @param format The format of the texture to attach.
         * @param filters The texture parameters for filtering and wrapping.
         * @return Framebuffer& The framebuffer object itself for method chaining.
         */
        Framebuffer& attach_colour(TextureFormat format, const TextureParameters& filters = {});

        /**
         * @brief Create the framebuffer as a multisample anti-aliasing (MSAA) framebuffer.
         *
         * @return Framebuffer& The framebuffer object itself for method chaining.
         */
        Framebuffer& create_as_multisample_aa();

        Framebuffer& attach_renderbuffer();

        /**
         * @brief Attach a depth buffer to the framebuffer, e.g. for depth testing, shadow mapping,
         * etc.
         *
         * @return Framebuffer& The framebuffer object itself for method chaining.
         */
        Framebuffer& attach_depth_buffer();

        /**
         * @brief Attach a depth buffer array or cube map to the framebuffer, e.g for Cascaded
         * Shadow Maps.
         *
         * @return Framebuffer& The framebuffer object itself for method chaining.
         */
        Framebuffer& attach_depth_buffer_array();

        /**
         * @brief Attach a depth buffer cube map to the framebuffer, e.g. for environment mapping,
         * point light shadow mapping, etc.
         *
         * @return Framebuffer& The framebuffer object itself for method chaining.
         */
        Framebuffer& attach_depth_buffer_cube_map();

        [[nodiscard]] bool is_complete() const;

        const GLuint width = 0;
        const GLuint height = 0;

      private:
        std::vector<std::unique_ptr<TextureResource>> textures_;
        std::vector<GLuint> renderbuffers_;
    };

} // namespace gl
