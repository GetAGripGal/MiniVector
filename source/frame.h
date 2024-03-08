#pragma once

#include "shader.h"
#include "window.h"
#include <glad/glad.h>

#define MV_VERTEX_SHADER                                        \
    "#version 460 core\n"                                       \
    "// The vertices for the fullscreen quad\n"                 \
    "\n"                                                        \
    "const vec2 vertices[3] = {\n"                              \
    "    vec2(-1, -1),\n"                                       \
    "    vec2(3, -1),\n"                                        \
    "    vec2(-1, 3)\n"                                         \
    "};\n"                                                      \
    "// The texture coordinates\n"                              \
    "out vec2 uv;\n"                                            \
    "void main()\n"                                             \
    "{\n"                                                       \
    "   gl_Position = vec4(vertices[gl_VertexID], 0.0, 1.0);\n" \
    "   uv = 0.5 * gl_Position.xy + vec2(0.5);\n"               \
    "}\n"

#define MV_FRAGMENT_SHADER                  \
    "#version 460 core\n"                   \
    "\n"                                    \
    "uniform sampler2D frame;\n"            \
    "uniform vec2 resolution;\n"            \
    "in vec2 uv;\n"                         \
    "out vec4 FragColor;\n"                 \
    "\n"                                    \
    "void main()\n"                         \
    "{\n"                                   \
    "    FragColor = texture(frame, uv);\n" \
    "}\n"

#define MV_FRAGMENT_SHADER_CRT                                       \
    "#version 460 core\n"                                            \
    "#ifdef GL_ES\n"                                                 \
    "#define LOWP lowp\n"                                            \
    "precision mediump float;\n"                                     \
    "#else\n"                                                        \
    "#define LOWP\n"                                                 \
    "#endif\n"                                                       \
    "\n"                                                             \
    "uniform float CRT_CURVE_AMNTx;\n"                               \
    "uniform float CRT_CURVE_AMNTy;\n"                               \
    "#define CRT_CASE_BORDR 0.0125\n"                                \
    "#define SCAN_LINE_MULT 1250.0\n"                                \
    "\n"                                                             \
    "// varying LOWP vec4 v_color;\n"                                \
    "in vec2 uv;\n"                                                  \
    "\n"                                                             \
    "uniform sampler2D frame;\n"                                     \
    "\n"                                                             \
    "out vec4 FragColor;\n"                                          \
    "\n"                                                             \
    "void main() {\n"                                                \
    "    vec2 tc = vec2(uv.x, uv.y);\n"                              \
    "\n"                                                             \
    "    float dx = abs(0.5-tc.x);\n"                                \
    "    float dy = abs(0.5-tc.y);\n"                                \
    "\n"                                                             \
    "    dx *= dx;\n"                                                \
    "    dy *= dy;\n"                                                \
    "\n"                                                             \
    "    tc.x -= 0.5;\n"                                             \
    "    tc.x *= 1.0 + (dy * CRT_CURVE_AMNTx);\n"                    \
    "    tc.x += 0.5;\n"                                             \
    "\n"                                                             \
    "    tc.y -= 0.5;\n"                                             \
    "    tc.y *= 1.0 + (dx * CRT_CURVE_AMNTy);\n"                    \
    "    tc.y += 0.5;\n"                                             \
    "\n"                                                             \
    "    vec4 cta = texture(frame, vec2(tc.x, tc.y));\n"             \
    "\n"                                                             \
    "    cta.rgb += sin(tc.y * SCAN_LINE_MULT) * 0.02;\n"            \
    "\n"                                                             \
    "    if(tc.y > 1.0 || tc.x < 0.0 || tc.x > 1.0 || tc.y < 0.0)\n" \
    "        cta = vec4(0.0);\n"                                     \
    "\n"                                                             \
    "    FragColor = cta;\n"                                         \
    "}\n"

/**
 * @brief The framebuffer
 */
typedef struct mv_frame
{
    GLuint fbo;
    GLuint texture;
    mv_shader *shader;
    GLuint vao; // Remains empty and is only used to draw the fullscreen quad
} mv_frame;

/**
 * @brief Create the framebuffer
 * @param width The width of the framebuffer
 * @param height The height of the framebuffer
 * @return The framebuffer
 */
mv_frame *mv_create_frame(int32_t width, int32_t height);

/**
 * @brief Destroy the framebuffer
 * @param frame The framebuffer
 */
void mv_destroy_frame(mv_frame *frame);

/**
 * @brief Bind the framebuffer
 * @param frame The framebuffer
 */
void mv_bind_frame(mv_frame *frame);

/**
 * @brief Unbind the framebuffer
 */
void mv_unbind_frame();

/**
 * @brief Present the framebuffer
 */
void mv_present_frame(mv_frame *frame, mv_window *window);

/**
 * @brief Create the render texture
 * @param frame The framebuffer
 * @param width The width of the framebuffer
 * @param height The height of the framebuffer
 */
static void create_texture(mv_frame *frame, int32_t width, int32_t height);

/**
 * @brief Create the framebuffer
 * @param frame The framebuffer
 * @param width The width of the framebuffer
 * @param height The height of the framebuffer
 */
static void create_buffer(mv_frame *frame, int32_t width, int32_t height);

/**
 * @brief Create the vertex array object
 * @param frame The framebuffer
 */
static void create_vao(mv_frame *frame);