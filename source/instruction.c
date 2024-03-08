#include "instruction.h"
#include "log.h"

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
 * @param frame The frame to operate on
 * @param renderer The renderer to operate on
 */
void mv_process_instruction(mv_instruction_t instruction, mv_display *display, mv_frame *frame, mv_renderer *renderer)
{
    switch (instruction.type)
    {
    case MV_INSTRUCTION_CLEAR:
        mv_clear_display(display);
        mv_clear_buffers(renderer);
        break;
    case MV_INSTRUCTION_SET_POINT:
        mv_add_point(display, instruction.data >> 16, instruction.data & 0xFFFF);
        TRACE("Added point: %d, %d\n", instruction.data >> 16, instruction.data & 0xFFFF);
        break;
    default:
        WARN("Unknown instruction type: %u\n", instruction.type);
        break;
    }
}

/**
 * @brief Read an instruction from 5 bytes
 *
 * @param bytes The bytes to read from
 * @return mv_instruction_t The instruction read
 */
mv_instruction_t mv_read_instruction(uint8_t *bytes)
{
    mv_instruction_t instruction;
    instruction.type = bytes[0];
    instruction.data = (bytes[1] << 24) | (bytes[2] << 16) | (bytes[3] << 8) | bytes[4];
    return instruction;
}