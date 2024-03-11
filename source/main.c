#include <stdio.h>
#include <stdint.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <string.h>

#include "config.h"
#include "legacy/frame.h"
#include "window.h"
#include "legacy/display.h"
#include "legacy/renderer.h"
#include "instruction.h"
#include "gun.h"
#include "renderer.h"
#include "pipe.h"
#include "log.h"
#include "state.h"

/**
 * @brief The main loop for the legacy renderer
 *
 * @param state The state of the application
 */
void legacy_loop(mv_state *state);

/**
 * @brief The main loop for the modern renderer
 *
 * @param state The state of the application
 */
void modern_loop(mv_state *state);

int32_t main(int32_t argc, char *argv[])
{
    // For the legacy renderer
    mv_legacy_display *display_legacy = NULL;
    mv_legacy_renderer *renderer_legacy = NULL;
    mv_legacy_frame *frame_legacy = NULL;

    // For the modern renderer
    mv_electron_gun *electron_gun = NULL;
    mv_electron_renderer *electron_renderer = NULL;

    INFO("Starting the application\n");
    // Read the config
    mv_config config = mv_read_config(argc, argv);

    // Create the window and load the opengl context
    mv_window *window = mv_create_window(config.window.width, config.window.height, "MiniVector");

    // Create the framebuffer

    // Create the display
    if (config.legacy)
    {
        display_legacy = mv_create_legacy_display(config.resolution.width, config.resolution.height);
        renderer_legacy = mv_create_legacy_renderer(display_legacy);
        frame_legacy = mv_create_frame(config.resolution.width, config.resolution.height);
    }
    else
    {
        electron_gun = mv_create_electron_gun(100, 10, 10);
        electron_renderer = mv_create_electron_renderer(config.resolution.width, config.resolution.height);
    }

    // Open the pipe
    mv_pipe *pipe = mv_create_pipe(config.pipe);

    // Set the user pointer
    mv_state state = {
        .frame_legacy = frame_legacy,
        .window = window,
        .config = config,
        .pipe = pipe,
        .electron_gun = electron_gun,
        .electron_renderer = electron_renderer,
        .display_legacy = display_legacy,
        .renderer_legacy = renderer_legacy,
    };
    mv_set_window_user_pointer(window, &state);

    // The palette
    mv_color_t primary = config.palette.primary;
    mv_color_t secondary = config.palette.secondary;

    // The main loop
    TRACE("Starting the main loop\n");
    if (config.legacy)
    {
        legacy_loop(&state);
    }
    else
    {
        modern_loop(&state);
    }

    // Cleanup
    TRACE("Cleaning up\n");
    mv_destroy_window(window);
    if (config.legacy)
    {
        mv_destroy_legacy_display(display_legacy);
        mv_destroy_legacy_renderer(renderer_legacy);
        mv_destroy_legacy_frame(frame_legacy);
    }
    else
    {
        mv_destroy_electron_gun(electron_gun);
        mv_destroy_electron_renderer(electron_renderer);
    }
    mv_destroy_pipe(pipe);
    return 0;
}

void legacy_loop(mv_state *state)
{
    // The palette
    mv_color_t primary = state->config.palette.primary;
    mv_color_t secondary = state->config.palette.secondary;

    mv_instruction_t *instruction = NULL;

    // The main loop
    while (!glfwWindowShouldClose(state->window->glfw_ptr))
    {
        // Poll the pipe
        instruction = mv_poll_pipe(state->pipe);
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

void modern_loop(mv_state *state)
{
    mv_instruction_t *instruction = NULL;

    // The palette
    mv_color_t primary = state->config.palette.primary;
    mv_color_t secondary = state->config.palette.secondary;

    double now = glfwGetTime();
    double last = now;
    double delta = 0.0;

    // The main loop
    while (!glfwWindowShouldClose(state->window->glfw_ptr))
    {
        // Poll the pipe
        for (int32_t i = 0; i < MV_INSTRUCTION_PER_FRAME; i++)
        {
            instruction = mv_poll_pipe(state->pipe);
            if (!instruction)
            {
                break;
            }
            mv_process_instruction(instruction, state->electron_gun, state->electron_renderer);
            mv_update_electron_gun(state->electron_gun, delta);
            // Calculate the new position
            mv_calculate_pixels_electron_renderer(state->electron_renderer, state->electron_gun, primary, secondary);
        }

        // Render the electron gun
        mv_render_electron_gun(state->electron_renderer, state->electron_gun, state->window, primary, secondary);

        // Present the changes
        mv_present_window(state->window);

        // Update the time
        now = glfwGetTime();
        delta = now - last;
        last = now;

        // Fps counter
        char title[100];
        sprintf(title, "MiniVector | %f", 1.0 / delta);
        glfwSetWindowTitle(state->window->glfw_ptr, title);
    }
}