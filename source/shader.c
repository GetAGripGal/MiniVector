#include "shader.h"
#include "log.h"

#include <stdlib.h>

/**
 * @brief Create the shader
 * @return The shader
 */
mv_shader *mv_create_shader(const char *vertex_shader, const char *fragment_shader)
{
    mv_shader *shader = (mv_shader *)malloc(sizeof(mv_shader));
    shader->program = glCreateProgram();

    GLuint vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vertex_shader, NULL);
    glCompileShader(vertex);

    GLint success;
    glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        GLchar info_log[512];
        glGetShaderInfoLog(vertex, 512, NULL, info_log);
        MV_ERROR("Vertex shader compilation failed: %s\n", info_log);
    }

    GLuint fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fragment_shader, NULL);
    glCompileShader(fragment);

    glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        GLchar info_log[512];
        glGetShaderInfoLog(fragment, 512, NULL, info_log);
        MV_ERROR("Fragment shader compilation failed: %s\n", info_log);
    }

    glAttachShader(shader->program, vertex);
    glAttachShader(shader->program, fragment);
    glLinkProgram(shader->program);

    glGetProgramiv(shader->program, GL_LINK_STATUS, &success);
    if (!success)
    {
        GLchar info_log[512];
        glGetProgramInfoLog(shader->program, 512, NULL, info_log);
        MV_ERROR("Shader linking failed: %s\n", info_log);
    }

    glDeleteShader(vertex);
    glDeleteShader(fragment);

    return shader;
}

/**
 * @brief Destroy the shader
 * @param shader The shader
 */
void mv_destroy_shader(mv_shader *shader)
{
    glDeleteProgram(shader->program);
    free(shader);
}

/**
 * @brief Use the shader
 * @param shader The shader
 */
void mv_use_shader(mv_shader *shader)
{
    glUseProgram(shader->program);
}

/**
 * @brief Set the uniform vec2
 * @param shader The shader
 * @param name The name of the uniform
 * @param value The value of the uniform
 */
void mv_set_uniform_vec2(mv_shader *shader, const char *name, float x, float y)
{
    GLint location = glGetUniformLocation(shader->program, name);
    glUniform2f(location, x, y);
}

/**
 * @brief Set the uniform ivec2
 * @param shader The shader
 * @param name The name of the uniform
 * @param value The value of the uniform
 */
void mv_set_uniform_ivec2(mv_shader *shader, const char *name, int32_t x, int32_t y)
{
    GLint location = glGetUniformLocation(shader->program, name);
    glUniform2i(location, x, y);
}

/**
 * @brief Set the uniform mat4
 *
 * @param shader The shader
 * @param name The name of the uniform
 * @param value The value of the uniform
 */
void mv_set_uniform_mat4(mv_shader *shader, const char *name, float *value)
{
    GLint location = glGetUniformLocation(shader->program, name);
    glUniformMatrix4fv(location, 1, GL_FALSE, value);
}

/**
 * @brief Set the uniform vec3
 *
 * @param shader The shader
 * @param name The name of the uniform
 * @param x The x value of the uniform
 * @param y The y value of the uniform
 * @param z The z value of the uniform
 */
void mv_set_uniform_vec3(mv_shader *shader, const char *name, float x, float y, float z)
{
    GLint location = glGetUniformLocation(shader->program, name);
    glUniform3f(location, x, y, z);
}