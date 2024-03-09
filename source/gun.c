#include "gun.h"
#include "log.h"

#include <stdlib.h>
#include <math.h>

/**
 * @brief Initializes the electron gun
 *
 * @param movement_speed Speed of the electron beam in p/s
 * @param power_increase Speed of the electron beam power on in %/s
 * @param power_depletion Speed of the electron beam power off in %/s
 * @return Returns a new electron gun
 */
mv_electron_gun *mv_create_electron_gun(float movement_speed, float power_increase, float power_depletion)
{
    mv_electron_gun *gun = malloc(sizeof(mv_electron_gun));
    gun->position = (mv_point_t){0, 0};
    gun->target = (mv_point_t){0, 0};
    gun->movement_speed = movement_speed;
    gun->power_depletion = power_depletion;
    gun->power_increase = power_increase;
    gun->power = 0;
    gun->powered_on = 0;
    return gun;
}

/**
 * @brief Powers on the electron gun
 *
 * @param gun The electron gun to power on
 */
void mv_power_on_electron_gun(mv_electron_gun *gun)
{
    gun->powered_on = 1;
}

/**
 * @brief Powers off the electron gun
 *
 * @param gun The electron gun to power off
 */
void mv_power_off_electron_gun(mv_electron_gun *gun)
{
    gun->powered_on = 0;
}

/**
 * @brief Aims the electron gun
 *
 * @param gun The electron gun to aim
 * @param target The target to aim at
 */
void mv_aim_electron_gun(mv_electron_gun *gun, mv_point_t target)
{
    gun->target = target;
}

/**
 * @brief Updates the state of the electron gun each frame
 *
 * @param gun The electron gun to update
 * @param delta The time since the last frame
 */
void mv_update_electron_gun(mv_electron_gun *gun, float delta)
{
    // Store the previous position
    gun->prev_position = gun->position;

    // Update the position of the electron gun
    mv_point_t direction = mv_subtract_points(gun->target, gun->position);
    float distance = mv_magnitude_point(direction);

    if (floor(distance) > 2)
    {
        gun->moving = 1;
        mv_point_t normalized = mv_normalize_point(direction);
        mv_point_t scaled = mv_scale_point(normalized, gun->movement_speed * delta);
        mv_point_t new_position = mv_add_points(gun->position, scaled);

        // Check if the new position would overshoot the target
        if (mv_magnitude_point(mv_subtract_points(gun->target, new_position)) > distance)
        {
            // If it would overshoot, just set the position to the target
            gun->position = gun->target;
        }
        else
        {
            gun->position = new_position;
        }
    }
    else
    {
        gun->moving = 0;
    }

    // Update the power of the electron gun
    if (gun->powered_on)
    {
        gun->power += gun->power_increase * delta;
        if (gun->power > 100)
        {
            gun->power = 100;
        }
    }
    else
    {
        gun->power -= gun->power_depletion * delta;
        if (gun->power < 0)
        {
            gun->power = 0;
        }
    }
}

/**
 * @brief Destroys the electron gun
 *
 * @param gun The electron gun to destroy
 */
void mv_destroy_electron_gun(mv_electron_gun *gun)
{
    free(gun);
}