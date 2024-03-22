#include "win96.h"

#ifdef _WIN96

#include "../renderer.h"
#include "../log.h"
#include "../window.h"

#include <stdlib.h>

/**
 * @brief Initializes the Windows 96 support library
 *
 * @param state The state to initialize
 */
void mv_support_win96_init(mv_state *state)
{
    state->electron_position_buffer = malloc(sizeof(mv_electron_point) * state->config.executor.instruction_per_frame);
}

/**
 * @brief The emscripten-compliant event loop for Windows 96
 */
void mv_support_win96_event_loop(mv_state *state)
{
    uint64_t instruction_per_frame = state->config.executor.instruction_per_frame;

    uint64_t num_positions = 0;
    mv_instruction_t *instruction = NULL;
    gui_alertbox("Test", "This is a test message", "OK", 0);
    for (int64_t i = 0; i < instruction_per_frame; i++)
    {
        instruction = mv_read_instruction_from_pipe(state->pipe);
        if (!instruction)
        {
            break;
        }
        mv_process_instruction(instruction, state->electron_gun, state->electron_renderer);
        mv_update_electron_gun(state->electron_gun);
        // Remove the first position if it is too large
        if (num_positions + 1 > instruction_per_frame)
        {
            for (int64_t j = 0; j < num_positions - 1; j++)
            {
                state->electron_position_buffer[j] = state->electron_position_buffer[j + 1];
            }
            num_positions--;
        }
        state->electron_position_buffer[num_positions++] = (mv_electron_point){
            .position = (mv_point_t){state->electron_gun->position.x, state->electron_gun->position.y},
            .powered_on = state->electron_gun->powered_on,
        };
    }

    mv_calculate_pixels_electron_renderer(state->electron_renderer, state->electron_gun, state->config.palette.primary, state->config.palette.secondary, state->electron_position_buffer, num_positions);

    // Render the electron gun
    mv_render_electron_gun(state->electron_renderer, state->electron_gun, state->window, state->config.palette.primary, state->config.palette.secondary);

    // Present the changes
    mv_present_window(state->window);
}

#endif // _WIN96