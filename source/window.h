#pragma once
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>

#define OPENGL_VERSION_STRING "#version 460 core"

/**
 * @brief The window
 */
typedef struct mv_window
{
    GLFWwindow *glfw_ptr;
    struct
    {
        uint32_t width;
        uint32_t height;
    } reported_size;
} mv_window;

/**
 * @brief Create a window
 * @param width The width of the window
 * @param height The height of the window
 * @param title The title of the window
 * @return The window
 */
mv_window *mv_create_window(uint32_t width, uint32_t height, const char *title);

/**
 * @brief Destroy the window
 * @param window The window
 */
void mv_destroy_window(mv_window *window);

/**
 * @brief Present the window
 * @param window The window
 */
void mv_present_window(mv_window *window);

/**
 * @brief Set the window user pointer
 * @param window The window
 * @param ptr The pointer
 */
void mv_set_window_user_pointer(mv_window *window, void *ptr);

/**
 * @brief Get the window user pointer
 * @param window The window
 * @return The pointer
 */
void *mv_get_window_user_pointer(mv_window *window);

/**
 * @brief The resize callback
 */
static void callback_resize(GLFWwindow *window, int32_t width, int32_t height);

/**
 * @brief The error callback
 */
static void callback_error(int32_t error, const char *description);

/**
 * @brief The key callback
 */
static void callback_key(GLFWwindow *window, int32_t key, int32_t scancode, int32_t action, int32_t mods);