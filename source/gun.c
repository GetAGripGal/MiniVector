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
mv_electron_gun *mv_create_electron_gun(void)
{
    mv_electron_gun *gun = malloc(sizeof(mv_electron_gun));
    gun->position = (mv_point_t){0, 0};
    gun->target = (mv_point_t){0, 0};
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
    gun->position = gun->target;
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