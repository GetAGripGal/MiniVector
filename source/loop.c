#include "state.h"
#include "loop.h"
#include "log.h"
#include <stdlib.h>

#ifdef __unix__
#include <unistd.h>
#elif defined(_WIN32)
#include <windows.h>
#endif

#ifdef _WIN96
#include <win96/gui.h>
#endif

/**
 * @brief Initializes the modern application state
 *
 * @param state The state of the application
 */
void mv_init_modern_state(mv_state *state)
{

    // Create the electron renderer
    state->electron_gun = mv_create_electron_gun();
#ifndef _WIN96
    state->electron_renderer = mv_create_electron_renderer(state->config.resolution.width, state->config.resolution.height);
#endif

    // Update the electron gun
    state->electron_gun->radius = state->config.gun.radius;
    state->electron_gun->dim_factor = state->config.gun.dim_factor;
}

/**
 * @brief Destroys the modern application state
 *
 * @param state The state of the application
 */
void mv_destroy_modern_state(mv_state *state)
{
    // Destroy the electron renderer
    mv_destroy_electron_gun(state->electron_gun);
#ifndef _WIN96
    mv_destroy_electron_renderer(state->electron_renderer);
#endif
}

/**
 * @brief The main loop for the modern renderer
 *
 * @param state The state of the application
 */
void mv_modern_loop(mv_state *state)
{
    // The palette
    mv_color_t primary = state->config.palette.primary;
    mv_color_t secondary = state->config.palette.secondary;

    // The instructions per frame
    uint32_t instructions_per_frame = state->config.executor.instruction_per_frame;

    // The buffer of positions
    mv_electron_point *positions = calloc(instructions_per_frame + 1, sizeof(mv_electron_point));
    uint32_t num_positions = 0;

    // Delta time calculations
    double now = glfwGetTime();
    double last = now;
    double delta = 0.0;

    // For performance calculations
    double instruction_execution_time = 0;
    double compute_execution_time = 0;
    double render_execution_time = 0;
    double fps = 0;
    double frame_time = 0;

    MV_TRACE("Starting the modern loop\n");

    // The main loop
    while (!glfwWindowShouldClose(state->window->glfw_ptr))
    {
        now = glfwGetTime();
        delta = now - last;

        // Update the window title for performance benchmarks
        update_benchmark(state, instruction_execution_time, compute_execution_time, render_execution_time, &fps, &frame_time, now);

        // Delay the frame cap
        double frame_time = 1.0 / state->config.executor.frame_rate;
        delay_framecap(state, &delta, &now, &last, frame_time);

        // Execute instructions and keep track of the positions
        double instruction_execution_start = glfwGetTime();
        num_positions = 0;
        process_modern_instruction(state, positions, &num_positions);
        double instruction_execution_end = glfwGetTime();

// Calculate the new position
#ifndef _WIN96
        double compute_execution_start = glfwGetTime();
        mv_calculate_pixels_electron_renderer(state->electron_renderer, state->electron_gun, primary, secondary, positions, num_positions);
        double compute_execution_end = glfwGetTime();

        // Render the electron gun
        double render_execution_start = glfwGetTime();
        mv_render_electron_gun(state->electron_renderer, state->electron_gun, state->window, primary, secondary);
#endif

        // Present the changes
        mv_present_window(state->window);
        double render_execution_end = glfwGetTime();

        // Update the performance benchmarks
        instruction_execution_time = instruction_execution_end - instruction_execution_start;
#ifndef _WIN96
        compute_execution_time = compute_execution_end - compute_execution_start;
        render_execution_time = render_execution_end - render_execution_start;
#endif

        last = now;
        fps++;
    }
}

/**
 * @brief Delays the modern renderer to cap the frame rate
 *
 * @param state The state of the application
 * @param delta The time since the last frame
 * @param now The current time
 * @param last The last time
 * @param frame_time The time since the last frame
 */
static void delay_framecap(mv_state *state, double *delta, double *now, double *last, double frame_time)
{
    if (state->config.executor.frame_rate > 0)
    {
        while (*delta < frame_time)
        {
            double sleep_time = frame_time - *delta;
#ifdef __unix__
            usleep(sleep_time * 1000000);
#elif defined(_WIN32)
            Sleep(sleep_time * 1000);
#elif defined(_WIN96)
            emscripten_sleep(sleep_time * 1000);
#endif
            *now = glfwGetTime();
            *delta = *now - *last;
        }
    }
    else
    {
        *now = glfwGetTime();
        *delta = *now - *last;
    }
}

/**
 * @brief Processes the modern renderer instructions
 *
 * @param state The state of the application
 * @param positions The buffer of positions
 * @param num_positions The number of positions
 */
static void process_modern_instruction(mv_state *state, mv_electron_point *positions, uint32_t *num_positions)
{
    static mv_instruction_t *instruction = NULL;
    uint64_t instructions_per_frame = state->config.executor.instruction_per_frame;
    uint64_t instruction_index = 0;
    for (int32_t i = 0; i < instructions_per_frame; i++)
    {
        instruction = mv_read_instruction_from_pipe(state->pipe);
        if (!instruction)
        {
            break;
        }
        instruction_index++;
        mv_process_instruction(instruction, state->electron_gun, state->electron_renderer);
        mv_update_electron_gun(state->electron_gun);
        // Remove the first position if it is too large
        if (instruction_index >= instructions_per_frame)
        {
            for (int32_t j = 0; j < *num_positions - 1; j++)
            {
                positions[j] = positions[j + 1];
            }
            *num_positions -= 1;
        }
        positions[instruction_index] = (mv_electron_point){
            .position = (mv_point_t){state->electron_gun->position.x, state->electron_gun->position.y},
            .powered_on = state->electron_gun->powered_on,
        };
        *num_positions += 1;
        MV_TRACE("Position: %f, %f\n", positions[*num_positions - 1].position.x, positions[*num_positions - 1].position.y);
    }
}

/**
 * @brief Update the window title for performance benchmarks
 *
 * @param state The state of the application
 * @param instruction_execution_time The time to execute the instructions
 * @param compute_execution_time The time to compute the positions
 * @param render_execution_time The time to render the positions
 * @param fps The frames per second
 * @param frame_time The time since the last frame
 * @param now The current time
 */
static void update_benchmark(mv_state *state, double instruction_execution_time, double compute_execution_time, double render_execution_time, double *fps, double *frame_time, double now)
{
    static char title[256];
    if (now - *frame_time >= 1.0)
    {
        *frame_time = now;
        uint16_t instruction_execution_time_ms = instruction_execution_time * 1000;
        uint16_t compute_execution_time_ms = compute_execution_time * 1000;
        uint16_t render_execution_time_ms = render_execution_time * 1000;
        sprintf(title, "MiniVector | %ufps | instruction_execution: %ums | compute_execution: %ums | frame_execution: %ums", (uint16_t)*fps, instruction_execution_time_ms, compute_execution_time_ms, render_execution_time_ms);
        glfwSetWindowTitle(state->window->glfw_ptr, title);
        *fps = 0;
    }
}