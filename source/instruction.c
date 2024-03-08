#include "instruction.h"

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
    }
}