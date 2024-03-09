#include "frame.h"
#include "log.h"
#include "window.h"

#include <stdio.h>
#include <stdlib.h>

/**
 * @brief Create the framebuffer
 * @param width The width of the framebuffer
 * @param height The height of the framebuffer
 * @return The framebuffer
 */
mv_legacy_frame *mv_create_frame(int32_t width, int32_t height)
{
    mv_legacy_frame *frame = (mv_legacy_frame *)malloc(sizeof(mv_legacy_frame));
    frame->fbo = 0;
    frame->texture = 0;

    create_buffer(frame, width, height);
    create_texture(frame, width, height);
    create_vao(frame);

    TRACE("Compiling the framebuffer shader\n");
    frame->shader = mv_create_shader(MV_VERTEX_SHADER, MV_FRAGMENT_SHADER_CRT);
    INFO("Compiled framebuffer shader\n");

    GLenum error = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (error != GL_FRAMEBUFFER_COMPLETE)
    {
        ERROR("Framebuffer is not complete: %u\n", error);
        exit(1);
    }

    frame->resolution.width = width;
    frame->resolution.height = height;

    return frame;
}

/**
 * @brief Bind the framebuffer
 * @param frame The framebuffer
 */
void mv_bind_legacy_frame(mv_legacy_frame *frame)
{
    glBindFramebuffer(GL_FRAMEBUFFER, frame->fbo);
}

/**
 * @brief Unbind the framebuffer
 */
void mv_unbind_legacy_frame()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

/**
 * @brief Destroy the framebuffer
 * @param frame The framebuffer
 */
void mv_destroy_legacy_frame(mv_legacy_frame *frame)
{
    mv_destroy_shader(frame->shader);
    glDeleteFramebuffers(1, &frame->fbo);
    glDeleteTextures(1, &frame->texture);
    free(frame);
}

/**
 * @brief Clear the framebuffer texture
 *
 * @param frame The framebuffer
 * @param color The color to clear the texture
 */
void mv_clear_legacy_frame(mv_legacy_frame *frame)
{
    // Recreate the texture
    glBindTexture(GL_TEXTURE_2D, frame->texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, frame->resolution.width, frame->resolution.height, 0, GL_RGBA, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindImageTexture(0, frame->texture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
}

/**
 * @brief Present the framebuffer
 *
 * @param frame The framebuffer
 * @param window The window to present to
 * @param clear_color The color to clear the framebuffer
 */
void mv_present_legacy_frame(mv_legacy_frame *frame, mv_window *window, mv_color_t clear_color)
{
    glViewport(0, 0, window->reported_size.width, window->reported_size.height);
    glClearColor((float)clear_color.r / 255.0f, (float)clear_color.g / 255.0f, (float)clear_color.b / 255.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    mv_use_shader(frame->shader);
    mv_set_uniform_ivec2(frame->shader, "resolution", window->reported_size.width, window->reported_size.height);
    glBindVertexArray(frame->vao);
    glBindTexture(GL_TEXTURE_2D, frame->texture);
    glDrawArrays(GL_TRIANGLES, 0, 3);
}

/**
 * @brief Create the framebuffer
 * @param frame The framebuffer
 * @param width The width of the framebuffer
 * @param height The height of the framebuffer
 */
static void create_buffer(mv_legacy_frame *frame, int32_t width, int32_t height)
{
    GLuint fbo = 0;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    frame->fbo = fbo;
}

/**
 * @brief Create the render texture
 * @param frame The framebuffer
 * @param width The width of the framebuffer
 * @param height The height of the framebuffer
 */
static void create_texture(mv_legacy_frame *frame, int32_t width, int32_t height)
{
    GLuint texture = 0;
    glGenTextures(1, &texture);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
    glBindImageTexture(0, frame->texture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

    frame->texture = texture;
}

/**
 * @brief Create the vertex array object
 * @param frame The framebuffer
 */
static void create_vao(mv_legacy_frame *frame)
{
    GLuint vao = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    frame->vao = vao;
}