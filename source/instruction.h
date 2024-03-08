#pragma once

#include "display.h"

#include <stdint.h>

/**
 * @brief The instruction set
 */
enum
{
    MV_INSTRUCTION_CLEAR = 0x00000000,
    MV_INSTRUCTION_SET_POINT = 0x00000001,
};

/**
 * @brief An instruction send to the display
 */
typedef struct mv_instruction
{
    uint8_t type;
    uint32_t data;
} mv_instruction;

/**
 * @brief Process an instruction
 *
 * @param instruction The instruction to process
 * @param display The display to operate on
 */
void mv_process_instruction(mv_instruction instruction, mv_display *display);