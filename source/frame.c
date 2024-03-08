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
mv_frame *mv_create_frame(int32_t width, int32_t height)
{
    mv_frame *frame = (mv_frame *)malloc(sizeof(mv_frame));
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

    return frame;
}

/**
 * @brief Bind the framebuffer
 * @param frame The framebuffer
 */
void mv_bind_frame(mv_frame *frame)
{
    glBindFramebuffer(GL_FRAMEBUFFER, frame->fbo);
}

/**
 * @brief Unbind the framebuffer
 */
void mv_unbind_frame()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

/**
 * @brief Destroy the framebuffer
 * @param frame The framebuffer
 */
void mv_destroy_frame(mv_frame *frame)
{
    mv_destroy_shader(frame->shader);
    glDeleteFramebuffers(1, &frame->fbo);
    glDeleteTextures(1, &frame->texture);
    free(frame);
}

/**
 * @brief Present the framebuffer
 * @param frame The framebuffer
 */
void mv_present_frame(mv_frame *frame, struct mv_window *window)
{
    glViewport(0, 0, window->reported_size.width, window->reported_size.height);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
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
static void create_buffer(mv_frame *frame, int32_t width, int32_t height)
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
static void create_texture(mv_frame *frame, int32_t width, int32_t height)
{
    GLuint texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);

    frame->texture = texture;
}

/**
 * @brief Create the vertex array object
 * @param frame The framebuffer
 */
static void create_vao(mv_frame *frame)
{
    GLuint vao = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    frame->vao = vao;
}