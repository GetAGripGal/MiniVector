#include "window.h"
#include "log.h"
#include "state.h"

#include <stdlib.h>

/**
 * @brief Create a window
 * @param width The width of the window
 * @param height The height of the window
 * @param title The title of the window
 * @return The window
 */
mv_window *mv_create_window(uint32_t width, uint32_t height, const char *title)
{
    mv_window *window = (mv_window *)malloc(sizeof(mv_window));
    window->reported_size.width = width;
    window->reported_size.height = height;

    TRACE("Initializing GLFW\n");
    glfwSetErrorCallback(callback_error);
    if (glfwInit() == GLFW_FALSE)
    {
        ERROR("Failed to initialize GLFW\n");
        exit(1);
    }
    INFO("GLFW initialized\n");

    // Create the window
    TRACE("Creating glfw window | { width: %u, height: %u, title: %s }\n", width, height, title);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow *ptr = glfwCreateWindow(width, height, "MiniVector", NULL, NULL);
    if (!ptr)
    {
        ERROR("Failed to create window\n");
        glfwTerminate();
        exit(1);
    }
    window->glfw_ptr = ptr;

    INFO("Glfw window created\n");
    glfwMakeContextCurrent(window->glfw_ptr);
    glfwSwapInterval(0);

    // Load GLAD
    TRACE("Loading GLAD\n");
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        ERROR("Error: Failed to initialize GLAD\n");
        exit(1);
    }
    INFO("GLAD loaded\n");

    // Set the resize callback
    glfwSetFramebufferSizeCallback(window->glfw_ptr, callback_resize);
    // Set the key callback
    glfwSetKeyCallback(window->glfw_ptr, callback_key);

    INFO("Window created\n");
    return window;
}

/**
 * @brief Destroy the window
 * @param window The window
 */
void mv_destroy_window(mv_window *window)
{
    TRACE("Destroying window\n");
    glfwDestroyWindow(window->glfw_ptr);
    glfwTerminate();
    free(window);
}

/**
 * @brief Present the window
 * @param window The window
 */
void mv_present_window(mv_window *window)
{
    glViewport(0, 0, window->reported_size.width, window->reported_size.height);
    glfwSwapBuffers(window->glfw_ptr);
    glfwPollEvents();
}

/**
 * @brief Set the window user pointer
 * @param window The window
 * @param ptr The pointer
 */
void mv_set_window_user_pointer(mv_window *window, void *ptr)
{
    // Set as user pointer
    glfwSetWindowUserPointer(window->glfw_ptr, ptr);
}

/**
 * @brief Get the window user pointer
 * @param window The window
 * @return The pointer
 */
void *mv_get_window_user_pointer(mv_window *window)
{
    return glfwGetWindowUserPointer(window->glfw_ptr);
}

/**
 * @brief The resize callback
 */
static void callback_resize(GLFWwindow *window, int32_t width, int32_t height)
{
    mv_state *state = (mv_state *)glfwGetWindowUserPointer(window);
    state->window->reported_size.width = width;
    state->window->reported_size.height = height;

    mv_present_legacy_frame(state->frame_legacy, state->window, state->config.palette.primary);
}

/**
 * @brief The error callback
 */
static void callback_error(int32_t error, const char *description)
{
    ERROR("GLFW Error: %s\n", description);
    exit(1);
}

/**
 * @brief The key callback
 */
static void callback_key(GLFWwindow *window, int32_t key, int32_t scancode, int32_t action, int32_t mods)
{
    if (key == GLFW_KEY_F11 && action == GLFW_PRESS)
    {
        // Toggle fullscreen
        mv_state *state = (mv_state *)glfwGetWindowUserPointer(window);
        state->config.window.fullscreen = !state->config.window.fullscreen;
        if (state->config.window.fullscreen)
        {
            // Get the primary monitor
            GLFWmonitor *monitor = glfwGetPrimaryMonitor();
            const GLFWvidmode *mode = glfwGetVideoMode(monitor);
            glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
        }
        else
        {
            glfwSetWindowMonitor(window, NULL, 0, 0, state->config.resolution.width, state->config.resolution.height, 0);
        }
    }
}