#pragma once

#include "shader.h"
#include "gun.h"
#include "window.h"
#include "color.h"

#define STR_IMPL_(x) #x
#define STR(x) STR_IMPL_(x)
#define MV_ELECTRON_SHADER_DISPATCH_SIZE 8
#define MV_ELECTRON_SHADER_LAYOUT_CODE_STR "layout (local_size_x = " STR(MV_ELECTRON_SHADER_DISPATCH_SIZE) ", local_size_y = " STR(MV_ELECTRON_SHADER_DISPATCH_SIZE) ", local_size_z = 1) in;\n"
#define MV_ELECTRON_RENDERER_COMPUTE_SHADER                                                                                                    \
    "#version 460 core\n" MV_ELECTRON_SHADER_LAYOUT_CODE_STR                                                                                   \
    "layout (rgba32f, binding = 0) uniform image2D frame;\n"                                                                                   \
    "uniform vec3 primary_color;\n"                                                                                                            \
    "uniform vec3 secondary_color;\n"                                                                                                          \
    "\n"                                                                                                                                       \
    "uniform vec2 electron_gun_position;\n"                                                                                                    \
    "uniform bool electron_gun_power;\n"                                                                                                       \
    "uniform float electron_gun_radius;\n"                                                                                                     \
    "uniform int positions_count;\n"                                                                                                           \
    "uniform float electron_gun_dim_factor;\n"                                                                                                 \
    "\n"                                                                                                                                       \
    "struct mv_electron_point {\n"                                                                                                             \
    "   float x;\n"                                                                                                                            \
    "   float y;\n"                                                                                                                            \
    "   bool powered_on;\n"                                                                                                                    \
    "};\n"                                                                                                                                     \
    "\n"                                                                                                                                       \
    "layout(std430, binding = 0) buffer positions_buffer {\n"                                                                                  \
    "    mv_electron_point positions[];\n"                                                                                                     \
    "};\n"                                                                                                                                     \
    "\n"                                                                                                                                       \
    "bool is_point_between_two_points(vec2 point, vec2 a, vec2 b, float radius) {\n"                                                           \
    "    vec2 ap = point - a;\n"                                                                                                               \
    "    vec2 ab = b - a;\n"                                                                                                                   \
    "    \n"                                                                                                                                   \
    "    // Calculate square of length of ab\n"                                                                                                \
    "    float abLengthSq = dot(ab, ab);\n"                                                                                                    \
    "    \n"                                                                                                                                   \
    "    // If abLengthSq is 0, a and b are the same point, return false\n"                                                                    \
    "    if (abLengthSq == 0.0)\n"                                                                                                             \
    "        return false;\n"                                                                                                                  \
    "    \n"                                                                                                                                   \
    "    // Calculate parameter t along ab, clamped between 0 and 1\n"                                                                         \
    "    float t = clamp(dot(ap, ab) / abLengthSq, 0.0, 1.0);\n"                                                                               \
    "    \n"                                                                                                                                   \
    "    // Calculate closest point on ab to the point\n"                                                                                      \
    "    vec2 closest = a + t * ab;\n"                                                                                                         \
    "    \n"                                                                                                                                   \
    "    // Calculate distance between closest point and the point\n"                                                                          \
    "    float dist = distance(point, closest);\n"                                                                                             \
    "    \n"                                                                                                                                   \
    "    // Check if distance is within radius\n"                                                                                              \
    "    return dist <= radius;\n"                                                                                                             \
    "}\n"                                                                                                                                      \
    "\n"                                                                                                                                       \
    "vec4 dim_color(ivec2 pixel_coords, float power) {\n"                                                                                      \
    "   vec4 color = imageLoad(frame, pixel_coords);\n"                                                                                        \
    "   color.a -= power;\n"                                                                                                                   \
    "   color.b -= electron_gun_dim_factor / 10.0;\n"                                                                                          \
    "   if (color.a < 0.0) {\n"                                                                                                                \
    "       color.a = 0.0;\n"                                                                                                                  \
    "   }\n"                                                                                                                                   \
    "   if (color.b < 0.0) {\n"                                                                                                                \
    "       color.b = 0.0;\n"                                                                                                                  \
    "   }\n"                                                                                                                                   \
    "   return color;\n"                                                                                                                       \
    "}\n"                                                                                                                                      \
    "\n"                                                                                                                                       \
    "bool should_draw(vec2 pixel_coords) {\n"                                                                                                  \
    "   // If the point is in between the point behind it reached this frame, draw it\n"                                                       \
    "   for (int i = 0; i < positions_count-1; i++) {\n"                                                                                       \
    "       mv_electron_point point = positions[i];\n"                                                                                         \
    "       mv_electron_point next_point = positions[i + 1];\n"                                                                                \
    "       if (is_point_between_two_points(pixel_coords, vec2(point.x, point.y), vec2(next_point.x, next_point.y), electron_gun_radius)) {\n" \
    "           if (point.powered_on && next_point.powered_on) {\n"                                                                            \
    "               return true;\n"                                                                                                            \
    "           }\n"                                                                                                                           \
    "       }\n"                                                                                                                               \
    "   }\n"                                                                                                                                   \
    "   return false;\n"                                                                                                                       \
    "}\n"                                                                                                                                      \
    "\n"                                                                                                                                       \
    "void main() {\n"                                                                                                                          \
    "   float dim = electron_gun_dim_factor;\n"                                                                                                \
    "   ivec2 pixel_coords = ivec2(gl_GlobalInvocationID.xy);\n"                                                                               \
    "   ivec2 resolution = imageSize(frame).xy;\n"                                                                                             \
    "   //pixel_coords.y = resolution.y - pixel_coords.y;\n"                                                                                   \
    "   vec2 uv = vec2(pixel_coords) / vec2(resolution);\n"                                                                                    \
    "   vec2 diff = vec2(pixel_coords) - floor(electron_gun_position);\n"                                                                      \
    "   float distance = length(diff);\n"                                                                                                      \
    "   if ((distance < electron_gun_radius && electron_gun_power) || should_draw(vec2(pixel_coords))) {\n"                                    \
    "       float power = 1.0;//clamp(dim * distance, 0.0, 1.0);\n"                                                                            \
    "       imageStore(frame, pixel_coords, vec4(secondary_color, power));\n"                                                                  \
    "       return;\n"                                                                                                                         \
    "   }\n"                                                                                                                                   \
    "   vec4 dimmed = dim_color(pixel_coords, dim);\n"                                                                                         \
    "   imageStore(frame, pixel_coords, dimmed);\n"                                                                                            \
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

#define MV_ELECTRON_RENDERER_FRAGMENT_SHADER                                                                  \
    "#version 460 core\n"                                                                                     \
    "\n"                                                                                                      \
    "uniform sampler2D frame;\n"                                                                              \
    "uniform vec2 resolution;\n"                                                                              \
    "in vec2 uv;\n"                                                                                           \
    "out vec4 FragColor;\n" MV_ELECTRON_RENDERER_FRAGMENT_SHADER_BLOOM                                        \
        MV_ELECTRON_RENDERER_FRAGMENT_SHADER_BLUR                                                             \
    "\n"                                                                                                      \
    "vec4 sample_letterboxed(sampler2D texture_sampler, vec2 texture_coords) {\n"                             \
    "   ivec2 texture_resolution = textureSize(frame, 1);\n"                                                  \
    "   float texture_aspect = float(texture_resolution.x) / float(texture_resolution.y);\n"                  \
    "   float screen_aspect = resolution.x / resolution.y;\n"                                                 \
    "   float letterbox_width = 0.0;\n"                                                                       \
    "   float letterbox_height = 0.0;\n"                                                                      \
    "\n"                                                                                                      \
    "   if (texture_aspect < screen_aspect) {\n"                                                              \
    "       letterbox_height = (1.0 - texture_aspect / screen_aspect) / 2.0;\n"                               \
    "   } else {\n"                                                                                           \
    "       letterbox_width = (1.0 - screen_aspect / texture_aspect) / 2.0;\n"                                \
    "   }\n"                                                                                                  \
    "\n"                                                                                                      \
    "   if (texture_coords.x < letterbox_width || texture_coords.y > 1.0 - letterbox_width || \n"             \
    "       texture_coords.y < letterbox_height || texture_coords.y > 1.0 - letterbox_height) {\n"            \
    "       return vec4(0.0, 0.0, 0.0, 1.0);\n"                                                               \
    "   }\n"                                                                                                  \
    "   return texture(texture_sampler, texture_coords);\n"                                                   \
    "}\n"                                                                                                     \
    "\n"                                                                                                      \
    "vec4 smooth_pixel(sampler2D textureSampler, vec2 texCoords, vec2 textureSize) {\n"                       \
    "    vec2 texelSize = 1.0 / textureSize;\n"                                                               \
    "\n"                                                                                                      \
    "    vec4 color = vec4(0.0);\n"                                                                           \
    "    color += sample_letterboxed(textureSampler, texCoords + vec2(-texelSize.x, -texelSize.y)) * 0.25;\n" \
    "    color += sample_letterboxed(textureSampler, texCoords + vec2(texelSize.x, -texelSize.y)) * 0.25;\n"  \
    "    color += sample_letterboxed(textureSampler, texCoords + vec2(-texelSize.x, texelSize.y)) * 0.25;\n"  \
    "    color += sample_letterboxed(textureSampler, texCoords + vec2(texelSize.x, texelSize.y)) * 0.25;\n"   \
    "\n"                                                                                                      \
    "    return color;\n"                                                                                     \
    "}\n"                                                                                                     \
    "vec2 crt_curve(vec2 uv) {\n"                                                                             \
    "    uv = uv * 2.0 - 1.0;\n"                                                                              \
    "    vec2 offset = abs(uv.yx) / vec2(6.0, 4.0);\n"                                                        \
    "    uv = uv + uv * offset * offset;\n"                                                                   \
    "    uv = uv * 0.5 + 0.5;\n"                                                                              \
    "    return uv;\n"                                                                                        \
    "}\n"                                                                                                     \
    "\n"                                                                                                      \
    "vec4 crt_vignette(vec2 uv) {\n"                                                                          \
    "    float vignette = uv.x * uv.y * (1.0 - uv.x) * (1.0 - uv.y);\n"                                       \
    "    return vec4(vignette, vignette, vignette, 1.0);\n"                                                   \
    "}\n"                                                                                                     \
    "\n"                                                                                                      \
    "void main()\n"                                                                                           \
    "{\n"                                                                                                     \
    "   vec2 screen_uv = gl_FragCoord.xy / resolution;\n"                                                     \
    "   vec2 crt_uv = screen_uv = crt_curve(screen_uv);\n"                                                    \
    "   if (crt_uv.x < 0.0 || crt_uv.x > 1.0 || crt_uv.y < 0.0 || crt_uv.y > 1.0) {\n"                        \
    "        FragColor = vec4(0.0, 0.0, 0.0, 1.0);\n"                                                         \
    "        return;\n"                                                                                       \
    "   }\n"                                                                                                  \
    "   // Add crossair \n"                                                                                   \
    "   vec2 crossair = vec2(0.5, 0.5);\n"                                                                    \
    "   float crossair_size = 0.001;\n"                                                                       \
    "   // if (abs(crt_uv.x - crossair.x) < crossair_size || abs(crt_uv.y - crossair.y) < crossair_size) {\n" \
    "   //     FragColor = vec4(1.0, 1.0, 1.0, 1.0);\n"                                                       \
    "   //     return;\n"                                                                                     \
    "   // }\n"                                                                                               \
    "   vec4 color = smooth_pixel(frame, crt_uv, vec2(textureSize(frame, 1)));\n"                             \
    "   color = crt_vignette(crt_uv) * color;\n"                                                              \
    "   color = blur(crt_uv, 3);\n"                                                                           \
    "   color = clamp(color * 3.0, 0.0, 1.0);\n"                                                              \
    "   color = bloom(color, screen_uv);\n"                                                                   \
    "   FragColor = color;\n"                                                                                 \
    "}\n"

/**
 * @brief A point send to the gpu
 */
typedef struct mv_electron_point
{
    mv_point_t position;
    uint32_t powered_on;
} mv_electron_point;

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
    GLuint positions_buffer; // The buffer of positions

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
 * @param positions The positions of the electrons
 * @param positions_count The number of positions
 */
void mv_calculate_pixels_electron_renderer(mv_electron_renderer *renderer, mv_electron_gun *electron_gun, mv_color_t primary, mv_color_t secondary, mv_electron_point *positions, uint32_t positions_count);

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

/**
 * @brief Create the positions buffer
 *
 * @param positions The positions of the electrons
 * @param positions_count The number of positions
 * @return The buffer
 */
static GLuint create_positions_buffer();