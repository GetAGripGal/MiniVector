#include "renderer.h"
#include "log.h"

#include <stdlib.h>

/**
 * @brief Create the electron renderer
 *
 * @param width The width of the renderer
 * @param height The height of the renderer
 * @return The electron renderer
 */
mv_electron_renderer *mv_create_electron_renderer(uint32_t width, uint32_t height)
{
    mv_electron_renderer *renderer = malloc(sizeof(mv_electron_renderer));

    // Create the shader
    renderer->shader = mv_create_shader(MV_ELECTRON_RENDERER_VERTEX_SHADER, MV_ELECTRON_RENDERER_FRAGMENT_SHADER);

    // Create the compute shader
    renderer->compute_program = create_compute_shader(MV_ELECTRON_RENDERER_COMPUTE_SHADER);

    // Enable blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Create the vertex array object
    glGenVertexArrays(1, &renderer->vao);

    // Create the positions buffer
    renderer->positions_buffer = create_positions_buffer();

    // Create the frame textures
    renderer->frame_texture[0] = create_frame_buffer(width, height);
    renderer->frame_texture[1] = create_frame_buffer(width, height);
    renderer->current_texture = 0;

    // Set the resolution
    renderer->resolution.width = width;
    renderer->resolution.height = height;

    renderer->clear = 0;

    return renderer;
}

/**
 * @brief Destroy the electron renderer
 *
 * @param renderer The electron renderer
 */
void mv_destroy_electron_renderer(mv_electron_renderer *renderer)
{
    glDeleteProgram(renderer->compute_program);
    glDeleteTextures(1, &renderer->frame_texture[0]);
    glDeleteTextures(1, &renderer->frame_texture[1]);
    free(renderer);
}

/**
 * @brief Dispatch the compute shader and calculate the frame buffer
 *
 * @param renderer The electron renderer
 * @param electron_gun The electron gun
 * @param primary The primary color
 * @param secondary The secondary color
 * @param positions The positions of the electrons
 * @param positions_count The number of positions
 */
void mv_calculate_pixels_electron_renderer(mv_electron_renderer *renderer, mv_electron_gun *electron_gun, mv_color_t primary, mv_color_t secondary, mv_electron_point *positions, uint32_t positions_count)
{
    glUseProgram(renderer->compute_program);

    // Update the positions buffer
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, renderer->positions_buffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(mv_electron_point) * positions_count, positions, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, renderer->positions_buffer);

    // Bind the current texture
    glBindImageTexture(0, renderer->frame_texture[renderer->current_texture], 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
    glUniform3f(glGetUniformLocation(renderer->compute_program, "primary_color"), (float)primary.r / 255.0f, (float)primary.g / 255.0f, (float)primary.b / 255.0f);
    glUniform3f(glGetUniformLocation(renderer->compute_program, "secondary_color"), (float)secondary.r / 255.0f, (float)secondary.g / 255.0f, (float)secondary.b / 255.0f);

    glUniform1i(glGetUniformLocation(renderer->compute_program, "positions_count"), positions_count);
    glUniform2f(glGetUniformLocation(renderer->compute_program, "electron_gun_position"), electron_gun->position.x, electron_gun->position.y);
    glUniform1i(glGetUniformLocation(renderer->compute_program, "electron_gun_power"), electron_gun->powered_on);

    glDispatchCompute(renderer->resolution.width / MV_ELECTRON_SHADER_DISPATCH_SIZE, renderer->resolution.height / MV_ELECTRON_SHADER_DISPATCH_SIZE, 1);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);
}

/**
 * @brief Render the electron gun
 *
 * @param renderer The electron renderer
 * @param electron_gun The electron gun
 * @param window The window
 * @param primary The primary color
 * @param secondary The secondary color
 */
void mv_render_electron_gun(mv_electron_renderer *renderer, mv_electron_gun *electron_gun, mv_window *window, mv_color_t primary, mv_color_t secondary)
{
    if (renderer->clear)
    {
        TRACE("Clearing the frame\n");
        glBindTexture(GL_TEXTURE_2D, renderer->frame_texture[!renderer->current_texture]);

        // Clear the texture by filling it with zeros
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

        float *data = calloc(renderer->resolution.width * renderer->resolution.height * 4, sizeof(float));
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, renderer->resolution.width, renderer->resolution.height, GL_RGBA, GL_FLOAT, data);
        free(data);

        // Check for errors
        GLenum error = glGetError();
        if (error != GL_NO_ERROR)
        {
            ERROR("Failed to clear the frame: %d\n", error);
        }

        renderer->clear = 0;
    }

    glClearColor(((float)primary.r / 255.0f), ((float)primary.g / 255.0f), ((float)primary.b / 255.0f), 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Draw the frame
    mv_use_shader(renderer->shader);
    mv_set_uniform_vec2(renderer->shader, "resolution", window->reported_size.width, window->reported_size.height);
    glBindTexture(GL_TEXTURE_2D, renderer->frame_texture[renderer->current_texture]);
    glBindVertexArray(renderer->vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
}

/**
 * @brief Clear the frame
 *
 * @param renderer The electron renderer
 */
void mv_clear_frame(mv_electron_renderer *renderer)
{
    renderer->clear = 1;

    // Switch the current texture
    renderer->current_texture = !renderer->current_texture;
}

/**
 * @brief Create the compute shader
 *
 * @param source The source of the compute shader
 */
static GLuint create_compute_shader(const char *source)
{
    GLuint shader = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        GLchar info_log[512];
        glGetShaderInfoLog(shader, 512, NULL, info_log);
        ERROR("Compute shader compilation failed: %s\n", info_log);
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, shader);
    glLinkProgram(program);

    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success)
    {
        GLchar info_log[512];
        glGetProgramInfoLog(program, 512, NULL, info_log);
        ERROR("Program linking failed: %s\n", info_log);
    }

    glDeleteShader(shader);

    return program;
}

/**
 * @brief Create a frame buffer
 *
 * @param width The width of the frame buffer
 * @param height The height of the frame buffer
 * @return The frame buffer
 */
static GLuint create_frame_buffer(uint32_t width, uint32_t height)
{
    GLuint frame_buffer;

    // Create the frame texture
    glGenTextures(1, &frame_buffer);
    glBindTexture(GL_TEXTURE_2D, frame_buffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindImageTexture(0, frame_buffer, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

    return frame_buffer;
}

/**
 * @brief Create the positions buffer
 *
 * @param positions The positions of the electrons
 * @param positions_count The number of positions
 * @return The buffer
 */
static GLuint create_positions_buffer()
{
    GLuint buffer;
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, 0, NULL, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, buffer);
    return buffer;
}