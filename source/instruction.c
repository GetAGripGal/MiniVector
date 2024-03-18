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
 * @brief Process an instruction.
 *
 * @param instruction The instruction to process
 * @param electron_gun The electron gun to operate on
 * @param electron_renderer The electron renderer to operate on
 */
void mv_process_instruction(mv_instruction_t *instruction, mv_electron_gun *electron_gun, mv_electron_renderer *electron_renderer)
{
    TRACE("Processing instruction type: %u\n", instruction->type);

    switch (instruction->type)
    {
    case MV_INSTRUCTION_CLEAR:
        mv_clear_frame(electron_renderer);
        TRACE("Cleared frame\n");
        break;
    case MV_INSTRUCTION_SET_TARGET:
        mv_aim_electron_gun(electron_gun, (mv_point_t){instruction->data >> 16, instruction->data & 0xFFFF});
        TRACE("Aimed electron gun at %u, %u\n", instruction->data >> 16, instruction->data & 0xFFFF)
        break;
    case MV_INSTRUCTION_POWER_OFF:
        mv_power_off_electron_gun(electron_gun);
        TRACE("Powered off electron gun\n");
        break;
    case MV_INSTRUCTION_POWER_ON:
        mv_power_on_electron_gun(electron_gun);
        TRACE("Powered on electron gun\n");
        break;
    default:
        WARN("Unknown instruction type: %u\n", instruction->type);
        break;
    }
}

/**
 * @brief Process an instruction using the legacy display
 *
 * @param instruction The instruction to process
 * @param display The display to operate on
 * @param frame The frame to operate on
 * @param renderer The renderer to operate on
 */
void mv_process_instruction_legacy(mv_instruction_t *instruction, mv_legacy_display *display, mv_legacy_frame *frame, mv_legacy_renderer *renderer)
{
    switch (instruction->type)
    {
    case MV_LEGACY_INSTRUCTION_CLEAR:
        mv_clear_display_legacy(display);
        mv_legacy_renderer_clear_buffers(renderer);
        break;
    case MV_LEGACY_INSTRUCTION_SET_POINT:
        mv_add_point_legacy(display, instruction->data >> 16, instruction->data & 0xFFFF);
        break;
    default:
        WARN("Instruction type: %u unsupported in legacy\n", instruction->type);
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
    instruction.data = ((uint32_t)bytes[1] << 24) | (bytes[2] << 16) | (bytes[3] << 8) | bytes[4];
    return instruction;
}