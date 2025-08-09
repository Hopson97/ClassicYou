#include "Framebuffer.h"

#include <iostream>

namespace gl
{
    Framebuffer::Framebuffer(GLuint width, GLuint height)
        : width{width}
        , height{height}
    {
    }

    Framebuffer::Framebuffer(Framebuffer&& other) noexcept
        : GLResource(std::move(other))
        , textures_(std::move(other.textures_))
        , renderbuffers_(std::move(other.renderbuffers_))
    {
    }

    Framebuffer::~Framebuffer()
    {
        for (auto& renderbuffer : renderbuffers_)
        {
            glDeleteRenderbuffers(1, &renderbuffer);
        }
    }

    Framebuffer& Framebuffer::operator=(Framebuffer&& other) noexcept
    {
        destroy();
        id = other.id;
        other.id = 0;
        textures_ = std::move(other.textures_);
        renderbuffers_ = std::move(other.renderbuffers_);
        return *this;
    }

    void Framebuffer::bind(FramebufferTarget target, bool clear) const
    {
        assert(id);
        glViewport(0, 0, width, height);
        glBindFramebuffer(static_cast<GLenum>(target), id);

        if (clear)
        {
            // glClearColor(0, 0, 0, 0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        }
    }

    void Framebuffer::bind_texture(GLuint index, GLuint unit) const
    {
        assert(index < textures_.size());
        textures_[index]->bind(unit);
    }

    GLuint Framebuffer::get_texture_id(GLuint index) const
    {
        assert(index < textures_.size());
        return textures_[index]->id;
    }

    TextureResource& Framebuffer::get_texture(GLuint index)
    {
        assert(index < textures_.size());
        return *textures_[index];
    }

    Framebuffer& Framebuffer::attach_colour(TextureFormat format, const TextureParameters& filters)
    {
        assert(textures_.size() < GL_MAX_COLOR_ATTACHMENTS - 1);
        GLenum attachment = GL_COLOR_ATTACHMENT0 + static_cast<int>(textures_.size());

        auto texture = std::make_unique<Texture2D>();
        texture->create(width, height, 1, filters, format);
        glNamedFramebufferTexture(id, attachment, texture->id, 0);

        textures_.push_back(std::move(texture));
        return *this;
    }

    Framebuffer& Framebuffer::create_as_multisample_aa()
    {
        assert(textures_.size() < GL_MAX_COLOR_ATTACHMENTS - 1);
        GLenum attachment = GL_COLOR_ATTACHMENT0 + static_cast<int>(textures_.size());

        auto texture = std::make_unique<Texture2D>(Texture2DTarget::Multisample);
        texture->create_multi_sample(width, height, 2);

        glNamedFramebufferTexture(id, attachment, texture->id, 0);

        GLuint& rbo = renderbuffers_.emplace_back();
        glCreateRenderbuffers(1, &rbo);
        glNamedRenderbufferStorageMultisample(rbo, 2, GL_DEPTH_COMPONENT24, width, height);
        glNamedFramebufferRenderbuffer(id, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo);

        textures_.push_back(std::move(texture));
        return *this;
    }

    Framebuffer& Framebuffer::attach_renderbuffer()
    {
        GLuint& rbo = renderbuffers_.emplace_back();
        glCreateRenderbuffers(1, &rbo);
        glNamedRenderbufferStorage(rbo, GL_DEPTH_COMPONENT24, width, height);
        glNamedFramebufferRenderbuffer(id, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo);
        return *this;
    }

    Framebuffer& Framebuffer::attach_depth_buffer()
    {
        auto texture = std::make_unique<Texture2D>();
        texture->create_depth_texture(width, height);
        glNamedFramebufferTexture(id, GL_DEPTH_ATTACHMENT, texture->id, 0);

        // Tell OpenGL we don't want to draw to colour buffers
        glNamedFramebufferDrawBuffer(id, GL_NONE);
        glNamedFramebufferReadBuffer(id, GL_NONE);

        textures_.push_back(std::move(texture));
        return *this;
    }

    Framebuffer& Framebuffer::attach_depth_buffer_array()
    {
        auto texture = std::make_unique<Texture2DArray>();
        texture->create_depth_texture(width, height);
        glNamedFramebufferTexture(id, GL_DEPTH_ATTACHMENT, texture->id, 0);

        // Tell OpenGL we don't want to draw to colour buffers
        glNamedFramebufferDrawBuffer(id, GL_NONE);
        glNamedFramebufferReadBuffer(id, GL_NONE);

        textures_.push_back(std::move(texture));
        return *this;
    }

    Framebuffer& Framebuffer::attach_depth_buffer_cube_map()
    {
        auto texture = std::make_unique<CubeMapTexture>();
        texture->create_depth_cube(width, height);
        glNamedFramebufferTexture(id, GL_DEPTH_ATTACHMENT, texture->id, 0);

        // Tell OpenGL we don't want to draw to colour buffers
        glNamedFramebufferDrawBuffer(id, GL_NONE);
        glNamedFramebufferReadBuffer(id, GL_NONE);

        textures_.push_back(std::move(texture));
        return *this;
    }

    bool Framebuffer::is_complete() const
    {
        if (auto status =
                glCheckNamedFramebufferStatus(id, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
            std::println(std::cerr, "Framebuffer incomplete with status: {}.", status);
            return false;
        }
        return true;
    }
} // namespace gl
