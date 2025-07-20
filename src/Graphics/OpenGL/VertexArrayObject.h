#pragma once

#include <array>
#include <vector>

#include "BufferObject.h"
#include "GLResource.h"

namespace gl
{

    class VertexArrayObject : public GLResource<glCreateVertexArrays, glDeleteVertexArrays>
    {
      private:
        class AttributeBuilder
        {
          public:
            AttributeBuilder(GLuint& id, GLuint& attribs, GLuint vbo_binding_index);

            AttributeBuilder& add_instance_attribute(GLint size, GLenum type, GLuint offset,
                                                     GLuint divisor, int count, bool normalise = false);

            AttributeBuilder& add_attribute(GLint size, GLenum type, GLuint offset,
                                            bool normalise = false);

          private:
            GLuint vao_ = 0;
            GLuint* p_attribs_ = 0;
            GLuint vbo_binding_index_ = 0;
        };

      public:
        VertexArrayObject() = default;

        VertexArrayObject& operator=(VertexArrayObject&& other) noexcept;
        VertexArrayObject(VertexArrayObject&& other) noexcept;

        void bind() const;
        void reset() override;

        [[nodiscard]] AttributeBuilder add_vertex_buffer(const BufferObject& vbo, GLsizei stride);

      private:
        GLuint attribs_ = 0;
        GLuint vbo_count_ = 0;
    };
} // namespace gl