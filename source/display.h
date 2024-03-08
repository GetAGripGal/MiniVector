#pragma once

#define MV_MAX_DISPLAY_POINTS 4096

#include <stdint.h>

/**
 * @brief Represents a point in the XY plane
 */
typedef struct mv_point
{
    float x;
    float y;
} mv_point;

/**
 * @brief Simulates a XY display
 */
typedef struct mv_display
{
    mv_point points[MV_MAX_DISPLAY_POINTS];
    uint16_t point_count;
} mv_display;

/**
 * @brief Create the display
 */
mv_display *mv_create_display();

/**
 * @brief Set the line width
 *
 * @param display The display to set the line width
 * @param width The line width
 */
void mv_set_line_width(mv_display *display, uint16_t width);

/**
 * @brief Draws a line in the display
 *
 * @param display The display to draw the line
 * @param x0 The x coordinate of the start point
 * @param y0 The y coordinate of the start point
 * @param x1 The x coordinate of the end point
 * @param y1 The y coordinate of the end point
 */
void mv_draw_line(mv_display *display, int32_t x0, int32_t y0, int32_t x1, int32_t y1);

/**
 * @brief Clears the display
 *
 * @param display The display to clear
 */
void mv_clear_display(mv_display *display);
