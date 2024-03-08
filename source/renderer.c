#include "renderer.h"
#include "log.h"

#include <stdlib.h>
#include <HandmadeMath.h>

/**
 * @brief Create the renderer
 */
mv_renderer *mv_create_renderer()
{
    mv_renderer *renderer = malloc(sizeof(mv_renderer));

    renderer->shader = mv_create_shader(MV_LINE_VERTEX_SHADER, MV_LINE_FRAGMENT_SHADER);
    create_buffers(renderer);

    return renderer;
}

/**
 * @brief Set the renderer projection. The projection is orthographic
 *
 * @param renderer The renderer
 * @param width The width of the projection
 * @param height The height of the projection
 */
void mv_renderer_set_projection(mv_renderer *renderer, float width, float height)
{
    mv_use_shader(renderer->shader);
    HMM_Mat4 projection = HMM_Orthographic_LH_NO(0.0f, width, height, 0.0f, -1.0f, 1.0f);
    mv_set_uniform_mat4(renderer->shader, "projection", projection.Elements[0]);
}

/**
 * @brief Draw the display
 *
 * @param renderer The renderer
 * @param display The display to draw
 * @param primary The color of the background
 * @param secondary The color of the lines
 */
void mv_renderer_draw(mv_renderer *renderer, mv_display *display, mv_color_t primary, mv_color_t secondary)
{
    glClearColor(((float)primary.r / 255.0f), ((float)primary.g / 255.0f), ((float)primary.b / 255.0f), 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    mv_use_shader(renderer->shader);
    mv_set_uniform_vec3(renderer->shader, "color", ((float)secondary.r / 255.0f), ((float)secondary.g / 255.0f), ((float)secondary.b / 255.0f));

    glBindVertexArray(renderer->vao);
    glBindBuffer(GL_ARRAY_BUFFER, renderer->vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(mv_point) * display->point_count, display->points, GL_DYNAMIC_DRAW);
    glDrawArrays(GL_LINES, 0, display->point_count);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

/**
 * @brief Destroy the renderer
 *
 * @param renderer The renderer to destroy
 */
void mv_destroy_renderer(mv_renderer *renderer)
{
    mv_destroy_shader(renderer->shader);
    glDeleteVertexArrays(1, &renderer->vao);
    glDeleteBuffers(1, &renderer->vbo);
    free(renderer);
}

/**
 * @brief Create the buffers for the renderer
 */
static void create_buffers(mv_renderer *renderer)
{
    GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, 0, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(mv_point), (void *)0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    renderer->vao = vao;
    renderer->vbo = vbo;
}

/**
 * @brief Clear the buffers for the renderer
 *
 * @param renderer The renderer
 */
void mv_clear_buffers(mv_renderer *renderer)
{
    glBindVertexArray(renderer->vao);
    glBindBuffer(GL_ARRAY_BUFFER, renderer->vbo);
    glBufferData(GL_ARRAY_BUFFER, 0, NULL, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}