#pragma once

#include "shader.h"
#include "gun.h"
#include "window.h"
#include "color.h"

#define MV_ELECTRON_SHADER_DISPATCH_SIZE 16

#define MV_ELECTRON_RENDERER_COMPUTE_SHADER                                                                                                                \
    "#version 460 core\n"                                                                                                                                  \
    "layout (local_size_x = 16, local_size_y = 16, local_size_z = 1) in;\n"                                                                                \
    "layout (rgba32f, binding = 0) uniform image2D frame;\n"                                                                                               \
    "uniform vec3 primary_color;\n"                                                                                                                        \
    "uniform vec3 secondary_color;\n"                                                                                                                      \
    "\n"                                                                                                                                                   \
    "uniform vec2 electron_gun_previous_position;\n"                                                                                                       \
    "uniform vec2 electron_gun_position;\n"                                                                                                                \
    "uniform float electron_gun_power;\n"                                                                                                                  \
    "\n"                                                                                                                                                   \
    "bool is_point_between_two_points(vec2 point, vec2 a, vec2 b, float radius) {\n"                                                                       \
    "    vec2 ap = point - a;\n"                                                                                                                           \
    "    vec2 ab = b - a;\n"                                                                                                                               \
    "    \n"                                                                                                                                               \
    "    // Calculate square of length of ab\n"                                                                                                            \
    "    float abLengthSq = dot(ab, ab);\n"                                                                                                                \
    "    \n"                                                                                                                                               \
    "    // If abLengthSq is 0, a and b are the same point, return false\n"                                                                                \
    "    if (abLengthSq == 0.0)\n"                                                                                                                         \
    "        return false;\n"                                                                                                                              \
    "    \n"                                                                                                                                               \
    "    // Calculate parameter t along ab, clamped between 0 and 1\n"                                                                                     \
    "    float t = clamp(dot(ap, ab) / abLengthSq, 0.0, 1.0);\n"                                                                                           \
    "    \n"                                                                                                                                               \
    "    // Calculate closest point on ab to the point\n"                                                                                                  \
    "    vec2 closest = a + t * ab;\n"                                                                                                                     \
    "    \n"                                                                                                                                               \
    "    // Calculate distance between closest point and the point\n"                                                                                      \
    "    float dist = distance(point, closest);\n"                                                                                                         \
    "    \n"                                                                                                                                               \
    "    // Check if distance is within radius\n"                                                                                                          \
    "    return dist <= radius;\n"                                                                                                                         \
    "}\n"                                                                                                                                                  \
    "\n"                                                                                                                                                   \
    "vec4 dim_color(ivec2 pixel_coords) {\n"                                                                                                               \
    "   vec4 color = imageLoad(frame, pixel_coords);\n"                                                                                                    \
    "   color.a -= 0.005;\n"                                                                                                                               \
    "   if (color.a < 0.0) {\n"                                                                                                                            \
    "       color.a = 0.0;\n"                                                                                                                              \
    "   }\n"                                                                                                                                               \
    "   return color;\n"                                                                                                                                   \
    "}\n"                                                                                                                                                  \
    "\n"                                                                                                                                                   \
    "void main() {\n"                                                                                                                                      \
    "   ivec2 pixel_coords = ivec2(gl_GlobalInvocationID.xy);\n"                                                                                           \
    "   vec2 resolution = imageSize(frame);\n"                                                                                                             \
    "   vec2 diff = vec2(pixel_coords) - floor(electron_gun_position);\n"                                                                                  \
    "   float distance = length(diff);\n"                                                                                                                  \
    "   if (distance < 1 || is_point_between_two_points(vec2(pixel_coords), floor(electron_gun_position), floor(electron_gun_previous_position), .5)) {\n" \
    "       vec3 current_color = imageLoad(frame, pixel_coords).rgb;"                                                                                      \
    "       vec4 color = vec4(secondary_color, 1); // clamp(current_color + secondary_color * (electron_gun_power / 100), 0.0, 1.0);\n"                    \
    "       if (electron_gun_power > 0) {\n"                                                                                                               \
    "           imageStore(frame, pixel_coords, color);"                                                                                                   \
    "       }\n"                                                                                                                                           \
    "       return;\n"                                                                                                                                     \
    "   }\n"                                                                                                                                               \
    "   vec4 dimmed = dim_color(pixel_coords);\n"                                                                                                          \
    "   imageStore(frame, pixel_coords, dimmed);\n"                                                                                                        \
    "}\n"

#define MV_ELECTRON_RENDERER_VERTEX_SHADER                      \
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

#define MV_ELECTRON_RENDERER_FRAGMENT_SHADER_BLUR                                                   \
    "vec4 blur(vec2 pos, float r) {\n"                                                              \
    "   ivec2 texSize = textureSize(frame, 0);\n"                                                   \
    "   float x, y, xx, yy, rr = r * r, dx, dy, w, w0;\n"                                           \
    "   w0 = 0.3780 / pow(r, 1.975);\n"                                                             \
    "   vec2 p;\n"                                                                                  \
    "   vec4 col = vec4(0.0, 0.0, 0.0, 0.0);\n"                                                     \
    "   for (dx = 1.0 / texSize.x, x = -r, p.x = pos.x + (x * dx); x <= r; x++, p.x += dx) {\n"     \
    "       xx = x * x;\n"                                                                          \
    "       for (dy = 1.0 / texSize.y, y = -r, p.y = pos.y + (y * dy); y <= r; y++, p.y += dy) {\n" \
    "           yy = y * y;\n"                                                                      \
    "           if (xx + yy <= rr) {\n"                                                             \
    "               w = w0 * exp((-xx - yy) / (2.0 * rr));\n"                                       \
    "               col += bloom(texture(frame, p), p) * w;\n"                                      \
    "           }\n"                                                                                \
    "       }\n"                                                                                    \
    "   }\n"                                                                                        \
    "   return col;\n"                                                                              \
    "}\n"

#define MV_ELECTRON_RENDERER_FRAGMENT_SHADER_BLOOM                                    \
    "vec4 bloom(vec4 pixel, vec2 texel) {\n"                                          \
    "   vec4 col = vec4(0);\n"                                                        \
    "   float pixelWidth = 1;\n"                                                      \
    "   float pixelHeight = 1;\n"                                                     \
    "   float dim = .5;\n"                                                            \
    "   if (pixel.a < 1.0) {\n"                                                       \
    "       float glow = pixel.a * ((pixelWidth + pixelHeight) / 2.0);\n"             \
    "       vec4 bloom = vec4(0);\n"                                                  \
    "       bloom += (texture(frame, vec2(texel.x, texel.y)) - dim);\n"               \
    "       bloom += (texture(frame, vec2(texel.x - glow, texel.y - glow)) - dim);\n" \
    "       bloom += (texture(frame, vec2(texel.x + glow, texel.y + glow)) - dim);\n" \
    "       bloom += (texture(frame, vec2(texel.x + glow, texel.y - glow)) - dim);\n" \
    "       bloom += (texture(frame, vec2(texel.x - glow, texel.y + glow)) - dim);\n" \
    "       bloom += (texture(frame, vec2(texel.x + glow, texel.y)) - dim);\n"        \
    "       bloom += (texture(frame, vec2(texel.x - glow, texel.y)) - dim);\n"        \
    "       bloom += (texture(frame, vec2(texel.x, texel.y + glow)) - dim);\n"        \
    "       bloom += (texture(frame, vec2(texel.x, texel.y - glow)) - dim);\n"        \
    "       bloom = clamp(bloom / 9.0, 0.0, 1.0);\n"                                  \
    "       col = pixel + bloom;\n"                                                   \
    "   } else {\n"                                                                   \
    "       col = vec4(pixel.rgb, 1.0);\n"                                            \
    "   }\n"                                                                          \
    "   return col;\n"                                                                \
    "}\n"

#define MV_ELECTRON_RENDERER_FRAGMENT_SHADER                                                       \
    "#version 460 core\n"                                                                          \
    "\n"                                                                                           \
    "uniform sampler2D frame;\n"                                                                   \
    "uniform vec2 resolution;\n"                                                                   \
    "in vec2 uv;\n"                                                                                \
    "out vec4 FragColor;\n" MV_ELECTRON_RENDERER_FRAGMENT_SHADER_BLOOM                             \
        MV_ELECTRON_RENDERER_FRAGMENT_SHADER_BLUR                                                  \
    "\n"                                                                                           \
    "vec4 smoothPixel(sampler2D textureSampler, vec2 texCoords, vec2 textureSize) {\n"             \
    "    vec2 texelSize = 1.0 / textureSize;\n"                                                    \
    "\n"                                                                                           \
    "    vec4 color = vec4(0.0);\n"                                                                \
    "    color += texture(textureSampler, texCoords + vec2(-texelSize.x, -texelSize.y)) * 0.25;\n" \
    "    color += texture(textureSampler, texCoords + vec2(texelSize.x, -texelSize.y)) * 0.25;\n"  \
    "    color += texture(textureSampler, texCoords + vec2(-texelSize.x, texelSize.y)) * 0.25;\n"  \
    "    color += texture(textureSampler, texCoords + vec2(texelSize.x, texelSize.y)) * 0.25;\n"   \
    "\n"                                                                                           \
    "    return color;\n"                                                                          \
    "}\n"                                                                                          \
    "void main()\n"                                                                                \
    "{\n"                                                                                          \
    "   vec2 screen_uv = gl_FragCoord.xy / resolution;\n"                                          \
    "   vec4 color = smoothPixel(frame, screen_uv, resolution);\n"                                 \
    "   color = blur(screen_uv, 3);\n"                                                             \
    "   color = clamp(color * 7.0, 0.0, 1.0);\n"                                                   \
    "   // color = bloom(color, screen_uv);\n"                                                     \
    "   FragColor = color;\n"                                                                      \
    "}\n"

/**
 * @brief Handles the rendering of the electron gun
 */
typedef struct mv_electron_renderer
{
    GLuint compute_program;  // The compute program
    mv_shader *shader;       // The frame shader
    GLuint vao;              // The vertex array object
    GLuint frame_texture[2]; // The frame textures to render to (double buffered)
    int8_t current_texture;  // The current texture to render to

    struct
    {
        uint32_t width;
        uint32_t height;
    } resolution;
    int8_t clear; // Whether to clear the frame next cycle
} mv_electron_renderer;

/**
 * @brief Create the electron renderer
 *
 * @param width The width of the renderer
 * @param height The height of the renderer
 * @return The electron renderer
 */
mv_electron_renderer *mv_create_electron_renderer(uint32_t width, uint32_t height);

/**
 * @brief Destroy the electron renderer
 *
 * @param renderer The electron renderer
 */
void mv_destroy_electron_renderer(mv_electron_renderer *renderer);

/**
 * @brief Dispatch the compute shader and calculate the frame buffer
 *
 * @param renderer The electron renderer
 * @param electron_gun The electron gun
 * @param primary The primary color
 * @param secondary The secondary color
 */
void mv_calculate_pixels_electron_renderer(mv_electron_renderer *renderer, mv_electron_gun *electron_gun, mv_color_t primary, mv_color_t secondary);

/**
 * @brief Render the electron gun
 *
 * @param renderer The electron renderer
 * @param electron_gun The electron gun
 * @param window The window
 * @param primary The primary color
 * @param secondary The secondary color
 */
void mv_render_electron_gun(mv_electron_renderer *renderer, mv_electron_gun *electron_gun, mv_window *window, mv_color_t primary, mv_color_t secondary);

/**
 * @brief Clear the frame
 *
 * @param renderer The electron renderer
 */
void mv_clear_frame(mv_electron_renderer *renderer);

/**
 * @brief Create the compute shader
 *
 * @param source The source of the compute shader
 */
static GLuint create_compute_shader(const char *source);

/**
 * @brief Create a frame buffer
 *
 * @param width The width of the frame buffer
 * @param height The height of the frame buffer
 * @return The frame buffer
 */
static GLuint create_frame_buffer(uint32_t width, uint32_t height);