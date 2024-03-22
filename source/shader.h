#pragma once

#include <stdint.h>
#include <glad/glad.h>

/**
 * @brief The shader
 */
typedef struct mv_shader
{
    GLuint program;
} mv_shader;

/**
 * @brief Create the shader
 * @return The shader
 */
mv_shader *mv_create_shader(const char *vertex_shader, const char *fragment_shader);

/**
 * @brief Destroy the shader
 *
 * @param shader The shader
 */
void mv_destroy_shader(mv_shader *shader);

/**
 * @brief Set the uniform vec2
 *
 * @param shader The shader
 * @param name The name of the uniform
 * @param value The value of the uniform
 */
void mv_set_uniform_vec2(mv_shader *shader, const char *name, float x, float y);

/**
 * @brief Set the uniform ivec2
 *
 * @param shader The shader
 * @param name The name of the uniform
 * @param value The value of the uniform
 */
void mv_set_uniform_ivec2(mv_shader *shader, const char *name, int32_t x, int32_t y);

/**
 * @brief Set the uniform vec3
 *
 * @param shader The shader
 * @param name The name of the uniform
 * @param x The x value of the uniform
 * @param y The y value of the uniform
 * @param z The z value of the uniform
 */
void mv_set_uniform_vec3(mv_shader *shader, const char *name, float x, float y, float z);

/**
 * @brief Set the uniform mat4
 *
 * @param shader The shader
 * @param name The name of the uniform
 * @param value The value of the uniform
 */
void mv_set_uniform_mat4(mv_shader *shader, const char *name, float *value);

/**
 * @brief Use the shader
 *
 * @param shader The shader
 */
void mv_use_shader(mv_shader *shader);