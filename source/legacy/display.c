#include "display.h"
#include "log.h"

#include <stdlib.h>
#include <glad/glad.h>

/**
 * @brief Create the display
 */
mv_legacy_display *mv_create_legacy_display()
{
    mv_legacy_display *display = malloc(sizeof(mv_legacy_display));
    display->points = (mv_point_t *)calloc(MV_MAX_DISPLAY_POINTS, sizeof(mv_point_t));
    display->point_count = 0;
    return display;
}

/**
 * @brief Destroy the display
 *
 * @param display The display to destroy
 */
void mv_destroy_legacy_display(mv_legacy_display *display)
{
    free(display->points);
    free(display);
}

/**
 * @brief Set the line width
 *
 * @param display The display to set the line width
 * @param width The line width
 */
void mv_set_line_width_legacy(mv_legacy_display *display, uint16_t width)
{
    // Check the supported line width
    GLfloat range[2];
    glGetFloatv(GL_ALIASED_LINE_WIDTH_RANGE, range);

    if (width < range[0] || width > range[1])
    {
        ERROR("The current OpenGL context onlu supports the line width range: %f-%f\n", range[0], range[1]);
        return;
    }

    glLineWidth(width);
}

/**
 * @brief Add a point to the display
 *
 * @param display The display to add a point to
 * @param x The x coordinate of the point
 * @param y The y coordinate of the poiint
 */
void mv_add_point_legacy(mv_legacy_display *display, int16_t x, int16_t y)
{
    if (display->point_count >= MV_MAX_DISPLAY_POINTS)
    {
        return;
    }
    display->points[display->point_count++] = (mv_point_t){(float)x, (float)y};
}

/**
 * @brief Draws a line in the display
 *
 * @param display The display to draw the line
 * @param x0 The x coordinate of the start point
 * @param y0 The y coordinate of the start point
 * @param x1 The x coordinate of the end point
 * @param y1 The y coordinate of the end point
 */
void mv_draw_line_legacy(mv_legacy_display *display, int16_t x0, int16_t y0, int16_t x1, int16_t y1)
{
    if (display->point_count >= MV_MAX_DISPLAY_POINTS)
    {
        return;
    }

    display->points[display->point_count++] = (mv_point_t){(float)x0, (float)y0};
    display->points[display->point_count++] = (mv_point_t){(float)x1, (float)y1};
}

/**
 * @brief Clears the display
 *
 * @param display The display to clear
 */
void mv_clear_display_legacy(mv_legacy_display *display)
{
    display->points = (mv_point_t *)calloc(MV_MAX_DISPLAY_POINTS, sizeof(mv_point_t));
    display->point_count = 0;
}
