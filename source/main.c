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
    mv_draw_line(display, 100, 100, 120, 100);
    mv_draw_line(display, 100, 100, 95, 95);
    mv_draw_line(display, 120, 100, 125, 95);
    mv_draw_line(display, 105, 90, 105, 80);
    mv_draw_line(display, 115, 90, 115, 80);

    // Draw hello world

    // H
    mv_draw_line(display, 10, 10, 10, 20);
    mv_draw_line(display, 10, 15, 15, 15);
    mv_draw_line(display, 15, 10, 15, 20);

    // E
    mv_draw_line(display, 20, 10, 20, 20);
    mv_draw_line(display, 20, 11, 25, 11);
    mv_draw_line(display, 20, 15, 25, 15);
    mv_draw_line(display, 20, 19, 25, 19);

    // L
    mv_draw_line(display, 30, 10, 30, 20);
    mv_draw_line(display, 30, 19, 35, 19);

    // L
    mv_draw_line(display, 40, 10, 40, 20);
    mv_draw_line(display, 40, 19, 45, 19);

    // O
    mv_draw_line(display, 50, 10, 50, 20);
    mv_draw_line(display, 50, 11, 55, 11);
    mv_draw_line(display, 55, 10, 55, 20);
    mv_draw_line(display, 50, 19, 55, 19);

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
        mv_renderer_set_projection(renderer, config.resolution.width, config.resolution.height);

        mv_renderer_draw(renderer, display, primary, secondary);

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