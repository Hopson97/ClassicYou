#include "VertexArrayObject.h"

namespace gl
{
    VertexArrayObject& VertexArrayObject::operator=(VertexArrayObject&& other) noexcept
    {
        destroy();
        id = other.id;
        other.id = 0;

        attribs_ = other.attribs_;
        other.attribs_ = 0;

        vbo_count_ = other.vbo_count_;
        other.vbo_count_ = 0;

        return *this;
    }

    VertexArrayObject::VertexArrayObject(VertexArrayObject&& other) noexcept
        : GLResource(std::move(other))
        , attribs_(other.attribs_)
        , vbo_count_(other.vbo_count_)
    {
        other.attribs_ = 0;
        other.vbo_count_ = 0;
    }

    void VertexArrayObject::bind() const
    {
        assert(id);
        glBindVertexArray(id);
    }

    VertexArrayObject::AttributeBuilder
    VertexArrayObject::add_vertex_buffer(const BufferObject& vbo, GLsizei stride)
    {
        glVertexArrayVertexBuffer(id, vbo_count_, vbo.id, 0, stride);
        return {id, attribs_, vbo_count_++};
    }

    void VertexArrayObject::reset()
    {
        GLResource::destroy();
        GLResource::create();
        attribs_ = 0;
        vbo_count_ = 0;
    }

    VertexArrayObject::AttributeBuilder::AttributeBuilder(GLuint& id, GLuint& attribs,
                                                          GLuint vbo_binding_index)
        : vao_(id)
        , p_attribs_(&attribs)
        , vbo_binding_index_(vbo_binding_index)
    {
    }

    VertexArrayObject::AttributeBuilder&
    VertexArrayObject::AttributeBuilder::add_instance_attribute(GLint size, GLenum type,
                                                                GLuint offset, GLuint divisor,
                                                                int count)
    {
        for (int i = 0; i < 4; i++)
        {
            glEnableVertexArrayAttrib(vao_, *p_attribs_);
            glVertexArrayAttribFormat(vao_, *p_attribs_, size, type, GL_FALSE, offset * i);
            glVertexArrayAttribBinding(vao_, *p_attribs_, vbo_binding_index_);
            *p_attribs_ += 1;
        }
        glVertexArrayBindingDivisor(vao_, vbo_binding_index_, divisor);
        return *this;
    }

    VertexArrayObject::AttributeBuilder&
    VertexArrayObject::AttributeBuilder::add_attribute(GLint size, GLenum type, GLuint offset)
    {
        glEnableVertexArrayAttrib(vao_, *p_attribs_);
        glVertexArrayAttribFormat(vao_, *p_attribs_, size, type, GL_FALSE, offset);
        glVertexArrayAttribBinding(vao_, *p_attribs_, vbo_binding_index_);
        *p_attribs_ += 1;
        return *this;
    }
} // namespace gl