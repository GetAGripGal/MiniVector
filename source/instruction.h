#pragma once

#include "display.h"
#include "frame.h"
#include "renderer.h"

#include <stdint.h>

// The size of an instruction in bytes. Without struct padding
#define MV_INSTRUCTION_SIZE 5

/**
 * @brief The instruction set
 */
enum
{
    MV_INSTRUCTION_CLEAR = 0,
    MV_INSTRUCTION_SET_POINT = 1,
};

/**
 * @brief An instruction send to the display
 */
typedef struct mv_instruction_t
{
    uint32_t data;
    uint8_t type;
} mv_instruction_t;

/**
 * @brief Convert a set of coords into one uint32_t
 *
 * @param x The x coordinate
 * @param y The y coordinate
 */
uint32_t mv_coords_to_u32(int16_t x, int16_t y);

/**
 * @brief Process an instruction
 *
 * @param instruction The instruction to process
 * @param display The display to operate on
 * @param frame The frame to operate on
 * @param renderer The renderer to operate on
 */
void mv_process_instruction(mv_instruction_t instruction, mv_display *display, mv_frame *frame, mv_renderer *renderer);

/**
 * @brief Read an instruction from 5 bytes
 *
 * @param bytes The bytes to read from
 * @return mv_instruction_t The instruction read
 */
mv_instruction_t mv_read_instruction(uint8_t *bytes);
