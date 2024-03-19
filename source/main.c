#include <stdio.h>
#include <stdint.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

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

#ifdef _WIN96
#include <win96/wex.h>
#include <win96/gui.h>
#include <emscripten.h>

#include "support/win96.h"
#endif

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

    MV_INFO("Starting the application\n");
    // Read the config
    mv_config config = mv_read_config(argc, argv);

// Initialize the windows 96 window
#ifdef _WIN96
    set_proc_title("minivector");

    // Show hidden GL window
    wnd_set_title(UI_PHANDLE_WND_GL, "minivector");
    wnd_set_size(UI_PHANDLE_WND_GL, config.window.width, config.window.height);
    wnd_show(UI_PHANDLE_WND_GL);
#endif

    // Create the window and load the opengl context
    mv_window *window = mv_create_window(config.window.width, config.window.height, "MiniVector");

    // Create the framebuffer

    // Create the display
    if (config.legacy)
    {
        display_legacy = mv_create_legacy_display();
        renderer_legacy = mv_create_legacy_renderer();
        frame_legacy = mv_create_frame(config.resolution.width, config.resolution.height);
    }
    else
    {
        electron_gun = mv_create_electron_gun();
        electron_renderer = mv_create_electron_renderer(config.resolution.width, config.resolution.height);
    }

    // Open the pipe
    mv_pipe *pipe = mv_create_pipe(config.pipe, config.executor.instruction_per_frame);

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
    MV_TRACE("Starting the main loop\n");
    if (config.legacy)
    {
        legacy_loop(&state);
    }
    else
    {
        mv_start_pipe_thread(pipe);

#ifdef _WIN96
        mv_support_win96_init(&state);

        emscripten_set_main_loop_arg(mv_support_win96_event_loop, NULL, config.executor.frame_rate, 1);
#else
        modern_loop(&state);
#endif
        pthread_cancel(pipe->thread);
    }

    // Cleanup
    MV_TRACE("Cleaning up\n");
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

void modern_loop(mv_state *state)
{
    mv_instruction_t *instruction = NULL;

    // The palette
    mv_color_t primary = state->config.palette.primary;
    mv_color_t secondary = state->config.palette.secondary;

    // The instructions per frame
    uint32_t instructions_per_frame = state->config.executor.instruction_per_frame;

    // Update the electron gun
    state->electron_gun->radius = state->config.gun.radius;
    state->electron_gun->dim_factor = state->config.gun.dim_factor;

    // The buffer of positions
    mv_electron_point *positions = calloc(instructions_per_frame + 1, sizeof(mv_electron_point));
    uint32_t num_positions = 0;

    double now = glfwGetTime();
    double last = now;
    double delta = 0.0;

    // The title buffer
    char title[256];

    // For performance calculations
    double instruction_execution_time = 0;
    double compute_execution_time = 0;
    double render_execution_time = 0;
    double fps = 0;
    double frame_time = 0;

    // The main loop
    while (!glfwWindowShouldClose(state->window->glfw_ptr))
    {
        now = glfwGetTime();
        delta = now - last;

        // Set the title
        if (now - frame_time >= 1.0)
        {
            frame_time = now;
            sprintf(title, "MiniVector | %ufps | instruction_execution: %f | compute_execution: %f | frame_execution: %f", (uint16_t)fps, instruction_execution_time, compute_execution_time, render_execution_time);
            glfwSetWindowTitle(state->window->glfw_ptr, title);
            fps = 0;
        }

        // Frame cap
        double frame_time = 1.0 / state->config.executor.frame_rate;

        if (state->config.executor.frame_rate > 0)
        {
            while (delta < frame_time)
            {
                double sleep_time = frame_time - delta;
                usleep(sleep_time * 1000000);
                now = glfwGetTime();
                delta = now - last;
            }
        }

        // Execute instructions and keep track of the positions
        double instruction_execution_start = glfwGetTime();
        num_positions = 0;
        for (int32_t i = 0; i < instructions_per_frame; i++)
        {
            instruction = mv_read_instruction_from_pipe(state->pipe);
            if (!instruction)
            {
                break;
            }
            mv_process_instruction(instruction, state->electron_gun, state->electron_renderer);
            mv_update_electron_gun(state->electron_gun);
            // Remove the first position if it is too large
            if (num_positions + 1 > instructions_per_frame)
            {
                for (int32_t j = 0; j < num_positions - 1; j++)
                {
                    positions[j] = positions[j + 1];
                }
                num_positions--;
            }
            positions[num_positions++] = (mv_electron_point){
                .position = (mv_point_t){state->electron_gun->position.x, state->electron_gun->position.y},
                .powered_on = state->electron_gun->powered_on,
            };
        }
        double instruction_execution_end = glfwGetTime();

        // Calculate the new position
        double compute_execution_start = glfwGetTime();
        mv_calculate_pixels_electron_renderer(state->electron_renderer, state->electron_gun, primary, secondary, positions, num_positions);
        double compute_execution_end = glfwGetTime();

        // Render the electron gun
        double render_execution_start = glfwGetTime();
        mv_render_electron_gun(state->electron_renderer, state->electron_gun, state->window, primary, secondary);

        // Present the changes
        mv_present_window(state->window);
        double render_execution_end = glfwGetTime();

        instruction_execution_time = instruction_execution_end - instruction_execution_start;
        compute_execution_time = compute_execution_end - compute_execution_start;
        render_execution_time = render_execution_end - render_execution_start;

        last = now;
        fps++;
    }
}