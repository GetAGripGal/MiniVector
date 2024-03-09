#pragma once

#include "point.h"
#include <stdint.h>

#define MV_DEFAULT_OVERDRAW_FRAMES 10

/**
 * @brief Represents the state of the electron gun
 */
typedef struct mv_electron_gun
{
    mv_point_t position;      // The position of the gun
    mv_point_t prev_position; // The position of the gun in the last frame
    mv_point_t target;        // The target to aim at
    float movement_speed;     // Speed of the electron beam in p/s
    float power_depletion;    // Speed of the electron beam power off in %/s
    float power_increase;     // Speed of the electron beam power on in %/s
    int8_t power;             // 0-100% power
    int8_t powered_on;        // Whether the gun is powered on
    int8_t moving;            // Whether the gun is moving
} mv_electron_gun;

/**
 * @brief Initializes the electron gun
 *
 * @param movement_speed Speed of the electron beam in p/s
 * @param power_increase Speed of the electron beam power on in %/s
 * @param power_depletion Speed of the electron beam power off in %/s
 * @return Returns a new electron gun
 */
mv_electron_gun *mv_create_electron_gun(float movement_speed, float power_increase, float power_depletion);

/**
 * @brief Powers on the electron gun
 *
 * @param gun The electron gun to power on
 */
void mv_power_on_electron_gun(mv_electron_gun *gun);

/**
 * @brief Powers off the electron gun
 *
 * @param gun The electron gun to power off
 */
void mv_power_off_electron_gun(mv_electron_gun *gun);

/**
 * @brief Aims the electron gun
 *
 * @param gun The electron gun to aim
 * @param target The target to aim at
 */
void mv_aim_electron_gun(mv_electron_gun *gun, mv_point_t target);

/**
 * @brief Updates the state of the electron gun each frame
 *
 * @param gun The electron gun to update
 * @param delta The time since the last frame
 */
void mv_update_electron_gun(mv_electron_gun *gun, float delta);

/**
 * @brief Destroys the electron gun
 *
 * @param gun The electron gun to destroy
 */
void mv_destroy_electron_gun(mv_electron_gun *gun);