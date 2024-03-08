#include <stdio.h>
#include <stdint.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <string.h>

#include "config.h"
#include "frame.h"
#include "window.h"
#include "display.h"
#include "renderer.h"
#include "instruction.h"
#include "log.h"
#include "state.h"

int32_t main(int32_t argc, char *argv[])
{
    INFO("Starting the application\n");
    // Read the config
    mv_config config = mv_read_config(argc, argv);

    // Create the window and load the opengl context
    mv_window *window = mv_create_window(config.window.width, config.window.height, "MiniVector");

    // Create the framebuffer
    mv_frame *frame = mv_create_frame(config.resolution.width, config.resolution.height);

    // Create the display
    mv_display *display = mv_create_display(config.resolution.width, config.resolution.height);
    mv_set_line_width(display, config.line_width);

    // Draw a little smily face :)

    // The face in lines
    mv_process_instruction((mv_instruction){
                               .type = MV_INSTRUCTION_SET_POINT,
                               .data = mv_coords_to_u32(100, 100),
                           },
                           display);
    mv_process_instruction((mv_instruction){
                               .type = MV_INSTRUCTION_SET_POINT,
                               .data = mv_coords_to_u32(120, 100),
                           },
                           display);
    mv_process_instruction((mv_instruction){
                               .type = MV_INSTRUCTION_SET_POINT,
                               .data = mv_coords_to_u32(100, 100),
                           },
                           display);
    mv_process_instruction((mv_instruction){
                               .type = MV_INSTRUCTION_SET_POINT,
                               .data = mv_coords_to_u32(95, 95),
                           },
                           display);
    mv_process_instruction((mv_instruction){
                               .type = MV_INSTRUCTION_SET_POINT,
                               .data = mv_coords_to_u32(120, 100),
                           },
                           display);
    mv_process_instruction((mv_instruction){
                               .type = MV_INSTRUCTION_SET_POINT,
                               .data = mv_coords_to_u32(125, 95),
                           },
                           display);
    mv_process_instruction((mv_instruction){
                               .type = MV_INSTRUCTION_SET_POINT,
                               .data = mv_coords_to_u32(105, 90),
                           },
                           display);
    mv_process_instruction((mv_instruction){
                               .type = MV_INSTRUCTION_SET_POINT,
                               .data = mv_coords_to_u32(105, 80),
                           },
                           display);
    mv_process_instruction((mv_instruction){
                               .type = MV_INSTRUCTION_SET_POINT,
                               .data = mv_coords_to_u32(115, 90),
                           },
                           display);
    mv_process_instruction((mv_instruction){
                               .type = MV_INSTRUCTION_SET_POINT,
                               .data = mv_coords_to_u32(115, 80),
                           },
                           display);

    /// Create the renderer
    mv_renderer *renderer = mv_create_renderer(display);

    // Set the user pointer
    mv_state state = {
        .frame = frame,
        .window = window,
        .display = display,
        .renderer = renderer,
    };
    mv_set_window_user_pointer(window, &state);

    // The palette
    mv_color primary = config.palette.primary;
    mv_color secondary = config.palette.secondary;

    // The main loop
    TRACE("Starting the main loop\n");
    while (!glfwWindowShouldClose(window->glfw_ptr))
    {
        // Bind the framebuffer
        mv_bind_frame(frame);
        glViewport(0, 0, config.resolution.width, config.resolution.height);

        // Set the projection
        mv_renderer_set_projection(renderer, config.resolution.width, config.resolution.height);

        // Draw the lines
        mv_renderer_draw(renderer, display, primary, secondary);

        // Draw to the screen
        mv_unbind_frame();

        // Draw the framebuffer
        mv_present_frame(frame, window);

        // Present the changes
        mv_present_window(window);
    }

    // Cleanup
    TRACE("Cleaning up\n");
    mv_destroy_frame(frame);
    mv_destroy_window(window);
    return 0;
}