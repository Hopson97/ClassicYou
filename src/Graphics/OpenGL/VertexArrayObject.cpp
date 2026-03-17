#include "VertexArrayObject.h"

namespace gl
{
    constexpr void vertex_array_attrib_format(GLuint vao, GLuint attribute_index, GLint size,
                                              Type type, bool normalise, GLuint relative_offset)
    {
        if (!normalise &&
            (type == Type::Int || type == Type::UnsignedInt || type == Type::Short ||
             type == Type::UnsignedShort || type == Type::Byte || type == Type::UnsignedByte))
        {
            glVertexArrayAttribIFormat(vao, attribute_index, size, static_cast<GLenum>(type),
                                       relative_offset);
        }
        else
        {
            glVertexArrayAttribFormat(vao, attribute_index, size, static_cast<GLenum>(type),
                                      normalise ? GL_TRUE : GL_FALSE, relative_offset);
        }
    }

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
    VertexArrayObject::AttributeBuilder::add_instance_attribute(GLint size, Type type,
                                                                GLuint offset, GLuint divisor,
                                                                int count, bool normalise)
    {

        for (int i = 0; i < count /*4*/; i++)
        {
            glEnableVertexArrayAttrib(vao_, *p_attribs_);
            vertex_array_attrib_format(vao_, *p_attribs_, size, type,
                                       normalise ? GL_TRUE : GL_FALSE, offset * i);
            glVertexArrayAttribBinding(vao_, *p_attribs_, vbo_binding_index_);
            *p_attribs_ += 1;
        }
        glVertexArrayBindingDivisor(vao_, vbo_binding_index_, divisor);
        return *this;
    }

    VertexArrayObject::AttributeBuilder&
    VertexArrayObject::AttributeBuilder::add_attribute(GLint size, Type type, GLuint offset,
                                                       bool normalise)
    {
        glEnableVertexArrayAttrib(vao_, *p_attribs_);
        vertex_array_attrib_format(vao_, *p_attribs_, size, type, normalise ? GL_TRUE : GL_FALSE,
                                   offset);
        glVertexArrayAttribBinding(vao_, *p_attribs_, vbo_binding_index_);
        *p_attribs_ += 1;
        return *this;
    }
} // namespace gl
