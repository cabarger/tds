#ifndef TDS_GL_H
#include <GL/glx.h>

#define GL_UNIFORM_MATRIX_4FV(name) void name(GLint	location, GLsizei count, GLboolean transpose, const GLfloat *value)
typedef GL_UNIFORM_MATRIX_4FV(gl_uniform_matrix_4fv);

#define GL_GET_UNIFORM_LOCATION(name) GLint name(GLuint program, const GLchar *name)
typedef GL_GET_UNIFORM_LOCATION(gl_get_uniform_location);

#define GL_GEN_VERTEX_ARRAYS(name) void name(GLsizei n, GLuint *arrays)
typedef GL_GEN_VERTEX_ARRAYS(gl_gen_vertex_arrays);

#define GL_BIND_VERTEX_ARRAY(name) void name(GLuint array)
typedef GL_BIND_VERTEX_ARRAY(gl_bind_vertex_array);

#define GL_ENABLE_VERTEX_ATTRIB_ARRAY(name) void name(GLuint index)
typedef GL_ENABLE_VERTEX_ATTRIB_ARRAY(gl_enable_vertex_attrib_array);

#define GL_VERTEX_ATTRIB_POINTER(name) void name(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer)
typedef GL_VERTEX_ATTRIB_POINTER(gl_vertex_attrib_pointer);

#define GL_DELETE_SHADER(name) void name(GLuint shader)
typedef GL_DELETE_SHADER(gl_delete_shader);

#define GL_USE_PROGRAM(name) void name(GLuint program)
typedef GL_USE_PROGRAM(gl_use_program);

#define GL_GET_PROGRAM_IV(name) void name(GLuint program, GLenum pname, GLint *params)
typedef GL_GET_PROGRAM_IV(gl_get_program_iv);

#define GL_GET_PROGRAM_INFO_LOG(name) void name(GLuint program, GLsizei maxLength, GLsizei *length, GLchar *infoLog)
typedef GL_GET_PROGRAM_INFO_LOG(gl_get_program_info_log);

#define GL_LINK_PROGRAM(name) void name(GLuint program)
typedef GL_LINK_PROGRAM(gl_link_program);

#define GL_ATTACH_SHADER(name) void name(GLuint program, GLuint shader)
typedef GL_ATTACH_SHADER(gl_attach_shader);

#define GL_CREATE_PROGRAM(name) GLuint name(void)
typedef GL_CREATE_PROGRAM(gl_create_program);

#define GL_GET_SHADER_INFO_LOG(name) void name(GLuint shader, GLsizei maxLength, GLsizei *length, GLchar *infoLog)
typedef GL_GET_SHADER_INFO_LOG(gl_get_shader_info_log);

#define GL_GET_SHADER_IV(name) void name(GLuint shader, GLenum pname, GLint *params)
typedef GL_GET_SHADER_IV(gl_get_shader_iv);

/* https://docs.gl/gl3/glCompileShader */
#define GL_COMPILE_SHADER(name) void name(GLuint shader)
typedef GL_COMPILE_SHADER(gl_compile_shader);

/* https://docs.gl/gl3/glShaderSource */
#define GL_SHADER_SOURCE(name) void name(GLuint shader, GLsizei count, const GLchar **string, const GLint *length)
typedef GL_SHADER_SOURCE(gl_shader_source);

/* https://docs.gl/gl3/glCreateShader */
#define GL_CREATE_SHADER(name) GLuint name(GLenum shaderType);
typedef GL_CREATE_SHADER(gl_create_shader);

/* Create a new data store for a buffer object. The buffer object currently bound to target is used. */
#define GL_BUFFER_DATA(name) void name(GLenum target, GLsizeiptr size, const void *data, GLenum usage)
typedef GL_BUFFER_DATA(gl_buffer_data);

#define GL_BIND_BUFFER(name) void name(GLenum target, GLuint buffer)
typedef GL_BIND_BUFFER(gl_bind_buffer);

#define GL_GEN_BUFFERS(name) void name(GLsizei n, GLuint *buffers)
typedef GL_GEN_BUFFERS(gl_gen_buffers);

#define GL_ENABLE(name) void name(GLenum cap)
typedef GL_ENABLE(gl_enable);

#define GL_CLEAR_COLOR(name) void name(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
typedef GL_CLEAR_COLOR(gl_clear_color);

#define GL_CLEAR_(name) void name(GLbitfield mask)
typedef GL_CLEAR_(gl_clear);

#define GL_DRAW_ARRAYS(name) void name(GLenum mode, GLint first, GLsizei count);
typedef GL_DRAW_ARRAYS(gl_draw_arrays);

#define GL_FLUSH(name) void name(void)
typedef GL_FLUSH(gl_flush);

struct gl_api
{
    gl_gen_buffers *GenBuffers;
    gl_bind_buffer *BindBuffer;
    gl_buffer_data *BufferData;
    gl_create_shader *CreateShader;
    gl_shader_source *ShaderSource;
    gl_compile_shader *CompileShader;
    gl_get_shader_iv *GetShaderiv;
    gl_get_shader_info_log *GetShaderInfoLog;
    gl_create_program *CreateProgram;
    gl_attach_shader *AttachShader;
    gl_link_program *LinkProgram;
    gl_get_program_iv *GetProgramiv;
    gl_get_program_info_log *GetProgramInfoLog;
    gl_use_program *UseProgram;
    gl_delete_shader *DeleteShader;
    gl_vertex_attrib_pointer *VertexAttribPointer;
    gl_enable_vertex_attrib_array *EnableVertexAttribArray;
    gl_gen_vertex_arrays *GenVertexArrays;
    gl_bind_vertex_array *BindVertexArray;
    gl_get_uniform_location *GetUniformLocation;
    gl_uniform_matrix_4fv *UniformMatrix4fv;
    gl_enable *Enable;
    gl_clear_color *ClearColor;
    gl_clear *Clear;
    gl_draw_arrays *DrawArrays;
    gl_flush *Flush;
    //gl_polygon_mode *glPolygonMode;
};

#define TDS_GL_H
#endif
