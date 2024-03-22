#include <stdio.h>
#include <stdint.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <stdlib.h>
#include <string.h>

#ifdef __unix__
#include <unistd.h>
#include <pthread.h>
#endif
#ifdef _WIN32
#include <windows.h>
#endif

#include "config.h"
#include "legacy/frame.h"
#include "window.h"
#include "legacy/display.h"
#include "legacy/renderer.h"
#include "legacy/loop.h"
#include "instruction.h"
#include "gun.h"
#include "renderer.h"
#include "pipe.h"
#include "log.h"
#include "state.h"
#include "loop.h"

#ifdef _WIN96
#include <win96/wex.h>
#include <win96/gui.h>
#include <emscripten.h>

#include "support/win96.h"
#endif

int32_t main(int32_t argc, char *argv[])
{
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

// Open the pipe
#ifdef _WIN96
    mv_pipe *pipe = NULL;
#else
    mv_pipe *pipe = mv_create_pipe(config.pipe, config.executor.instruction_per_frame);
#endif

    // Set the user pointer
    mv_state state = {
        .window = window,
        .config = config,
        .pipe = pipe,
    };

    mv_set_window_user_pointer(window, &state);

    // The palette
    mv_color_t primary = config.palette.primary;
    mv_color_t secondary = config.palette.secondary;

    // The main loop
    MV_TRACE("Starting the main loop\n");

    if (config.legacy)
    {
        mv_start_pipe_thread(pipe);
        mv_init_legacy_state(&state);
        mv_legacy_loop(&state);
        mv_destroy_legacy_state(&state);
        mv_destroy_pipe(pipe);
    }
    else
    {
#ifndef _WIN96
        mv_start_pipe_thread(pipe);
#endif
        mv_init_modern_state(&state);
        mv_modern_loop(&state);
        mv_destroy_modern_state(&state);
#ifndef _WIN96
        mv_destroy_pipe(pipe);
#endif
    }

    // Cleanup
    MV_TRACE("Cleaning up\n");
    mv_destroy_window(window);
    return 0;
}