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
    win96_state = state;
    win96_position_buffer = malloc(sizeof(mv_electron_point) * state->config.executor.instruction_per_frame);
}

/**
 * @brief The emscripten-compliant event loop for Windows 96
 */
void mv_support_win96_event_loop()
{
    gui_alertbox("Test", "This is a test message", "OK", 1);

    uint64_t instruction_per_frame = win96_state->config.executor.instruction_per_frame;

    uint64_t num_positions = 0;
    mv_instruction_t *instruction = NULL;
    for (int64_t i = 0; i < instruction_per_frame; i++)
    {
        instruction = mv_read_instruction_from_pipe(win96_state->pipe);
        if (!instruction)
        {
            break;
        }
        mv_process_instruction(instruction, win96_state->electron_gun, win96_state->electron_renderer);
        mv_update_electron_gun(win96_state->electron_gun);
        // Remove the first position if it is too large
        if (num_positions + 1 > instruction_per_frame)
        {
            for (int64_t j = 0; j < num_positions - 1; j++)
            {
                win96_position_buffer[j] = win96_position_buffer[j + 1];
            }
            num_positions--;
        }
        win96_position_buffer[num_positions++] = (mv_electron_point){
            .position = (mv_point_t){win96_state->electron_gun->position.x, win96_state->electron_gun->position.y},
            .powered_on = win96_state->electron_gun->powered_on,
        };
    }

    mv_calculate_pixels_electron_renderer(win96_state->electron_renderer, win96_state->electron_gun, win96_state->config.palette.primary, win96_state->config.palette.secondary, win96_position_buffer, num_positions);

    // Render the electron gun
    mv_render_electron_gun(win96_state->electron_renderer, win96_state->electron_gun, win96_state->window, win96_state->config.palette.primary, win96_state->config.palette.secondary);

    // Present the changes
    mv_present_window(win96_state->window);
}

#endif // _WIN96