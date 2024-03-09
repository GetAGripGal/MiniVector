#pragma once

/**
 * @brief Represents a point in the XY plane
 */
typedef struct mv_point_t
{
    float x;
    float y;
} mv_point_t;

/**
 * @brief Creates a new point
 *
 * @param x The x coordinate
 * @param y The y coordinate
 * @return Returns a new point
 */
mv_point_t mv_create_point(float x, float y);

/**
 * @brief Subtracts two points
 *
 * @param a The first point
 * @param b The second point
 * @return Returns the difference between the two points
 */
mv_point_t mv_subtract_points(mv_point_t a, mv_point_t b);

/**
 * @brief Adds two points
 *
 * @param a The first point
 * @param b The second point
 * @return Returns the sum of the two points
 */
mv_point_t mv_add_points(mv_point_t a, mv_point_t b);

/**
 * @brief Multiplies a point by a scalar
 *
 * @param point The point
 * @param scalar The scalar
 * @return Returns the point multiplied by the scalar
 */
mv_point_t mv_multiply_point(mv_point_t point, float scalar);

/**
 * @brief Divides a point by a scalar
 *
 * @param point The point
 * @param scalar The scalar
 * @return Returns the point divided by the scalar
 */
mv_point_t mv_divide_point(mv_point_t point, float scalar);

/**
 * @brief Calculates the magnitude of a point
 *
 * @param point The point
 * @return Returns the magnitude of the point
 */
float mv_magnitude_point(mv_point_t point);

/**
 * @brief Normalizes a point
 *
 * @param point The point
 * @return Returns the normalized point
 */
mv_point_t mv_normalize_point(mv_point_t point);

/**
 * @brief Scales a point to a specific magnitude
 *
 * @param point The point
 * @param magnitude The magnitude
 * @return Returns the point scaled to the specific magnitude
 */
mv_point_t mv_scale_point(mv_point_t point, float magnitude);