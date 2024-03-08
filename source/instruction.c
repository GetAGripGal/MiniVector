#include "instruction.h"

/**
 * @brief Convert a set of coords into one uint32_t
 *
 * @param x The x coordinate
 * @param y The y coordinate
 */
uint32_t mv_coords_to_u32(int16_t x, int16_t y)
{
    return (x << 16) | y;
}

/**
 * @brief Process an instruction
 *
 * @param instruction The instruction to process
 * @param display The display to operate on
 */
void mv_process_instruction(mv_instruction instruction, mv_display *display)
{
    switch (instruction.type)
    {
    case MV_INSTRUCTION_CLEAR:
        mv_clear_display(display);
    case MV_INSTRUCTION_SET_POINT:
        mv_add_point(display, instruction.data >> 16, instruction.data & 0xFFFF);
    }
}