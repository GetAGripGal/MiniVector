#include "loop.h"
#include "legacy/display.h"
#include "legacy/frame.h"

/**
 * @brief Initializes the legacy application state
 *
 * @param state The state of the application
 */
void mv_init_legacy_state(mv_state *state)
{
    // Create the display
    state->display_legacy = mv_create_legacy_display();

    // Create the renderer
    state->renderer_legacy = mv_create_legacy_renderer();

    // Create the frame
    state->frame_legacy = mv_create_legacy_frame(state->config.resolution.width, state->config.resolution.height);
}

/**
 * @brief Destroys the legacy application state
 *
 * @param state The state of the application
 */
void mv_destroy_legacy_state(mv_state *state)
{
    // Destroy the display
    mv_destroy_legacy_display(state->display_legacy);

    // Destroy the renderer
    mv_destroy_legacy_renderer(state->renderer_legacy);

    // Destroy the frame
    mv_destroy_legacy_frame(state->frame_legacy);
}

/**
 * @brief The main loop for the legacy renderer
 *
 * @param state The state of the application
 */
void mv_legacy_loop(mv_state *state)
{
    // The palette
    mv_color_t primary = state->config.palette.primary;
    mv_color_t secondary = state->config.palette.secondary;

    mv_instruction_t *instruction = NULL;

    // The main loop
    while (!glfwWindowShouldClose(state->window->glfw_ptr))
    {
        // Poll the pipe
        instruction = mv_read_instruction_from_pipe(state->pipe);
        // Process the instruction
        if (instruction)
        {
            mv_process_instruction_legacy(instruction, state->display_legacy, state->frame_legacy, state->renderer_legacy);
        }

        // Bind the framebuffer
        mv_bind_legacy_frame(state->frame_legacy);
        glViewport(0, 0, state->config.resolution.width, state->config.resolution.height);

        // Set the projection
        mv_legacy_renderer_set_projection(state->renderer_legacy, state->config.resolution.width, state->config.resolution.height);

        // Draw the lines
        mv_legacy_renderer_draw(state->renderer_legacy, state->display_legacy, primary, secondary);

        // Draw to the screen
        mv_unbind_legacy_frame();

        // Draw the framebuffer
        mv_present_legacy_frame(state->frame_legacy, state->window, state->config.palette.primary);

        // Present the changes
        mv_present_window(state->window);
    }
}
