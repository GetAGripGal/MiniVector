//! Draws a fullscreen triangle with a red color.
struct VertexOutput {
    @builtin(position) position: vec4<f32>,
    @location(0) uv: vec2<f32>,
}

@vertex
fn vertex(@builtin(vertex_index) in_vertex_index: u32) -> VertexOutput {
    var out: VertexOutput;
    var vertices = array(
        vec2<f32>(-1, -1),
        vec2<f32>(3, -1),
        vec2<f32>(-1, 3),
    );
    out.position = vec4<f32>(vertices[in_vertex_index], 0.0, 1.0);
    out.uv = vertices[in_vertex_index] * 0.5 + 0.5;
    return out;
}

@group(0) @binding(0)
var t_diffuse: texture_2d<f32>;
@group(0) @binding(1)
var s_diffuse: sampler;

@group(1) @binding(0)
var<uniform> u_aspect: f32;

/// Letterbox the screen
fn letterbox(uv: vec2<f32>, screen_aspect: f32, texture_aspect: f32) -> vec2<f32> {
    var box_uv: vec2<f32> = uv;
    var scale: f32;
    if screen_aspect > texture_aspect {
        scale = texture_aspect / screen_aspect;
    } else {
        scale = screen_aspect / texture_aspect;
    }
    box_uv.x = (uv.x / scale + (1.0 - 1.0 / scale));
    box_uv.y = (uv.y / scale + (1.0 - 1.0 / scale));
    return box_uv;
}

/// Curve the screen like a crt display
fn crt_curve(uv: vec2<f32>) -> vec2<f32> {
    var crt_uv: vec2<f32> = uv * 2.0 - 1.0;
    let offset: vec2<f32> = abs(crt_uv.yx) / vec2<f32>(6.0, 4.0);
    crt_uv = crt_uv + crt_uv * offset * offset;
    crt_uv = crt_uv * 0.5 + 0.5;
    return crt_uv;
}

/// The crt vingette effect
fn crt_vignette(uv: vec2<f32>, falloff: f32, strength: f32) -> vec4<f32> {
    let vignette = uv.x * uv.y * pow((1.0 - uv.x), falloff) * pow((1.0 - uv.y), falloff);
    return vec4(vignette * strength, vignette * strength, vignette * strength, 1.0);
}

/// Add noise to the screen
fn crt_noise(uv: vec2<f32>, strength: f32) -> vec4<f32> {
    let noise = (fract(sin(dot(uv, vec2(12.9898, 78.233))) * 43758.5453) - 0.5) * strength;
    return vec4(noise, noise, noise, 1.0);
}

/// Smoothly read from texture
fn smooth_pixel(t_diffuse: texture_2d<f32>, s_diffuse: sampler, tex_coords: vec2<f32>, texture_size: vec2<u32>, strength: f32) -> vec4<f32> {
    let texel_size = vec2<f32>(1.0) / vec2<f32>(texture_size);

    var color: vec4<f32> = vec4<f32>(0.0);

    color += textureSample(t_diffuse, s_diffuse, tex_coords + vec2<f32>(-texel_size.x, -texel_size.y)) * strength;
    color += textureSample(t_diffuse, s_diffuse, tex_coords + vec2<f32>(texel_size.x, -texel_size.y)) * strength;
    color += textureSample(t_diffuse, s_diffuse, tex_coords + vec2<f32>(-texel_size.x, texel_size.y)) * strength;
    color += textureSample(t_diffuse, s_diffuse, tex_coords + vec2<f32>(texel_size.x, texel_size.y)) * strength;

    return color;
}

@fragment
fn fragment(in: VertexOutput) -> @location(0) vec4<f32> {
    let crt_uv = crt_curve(in.uv);
    if crt_uv.x < 0.0 || crt_uv.x > 1.0 || crt_uv.y < 0.0 || crt_uv.y > 1.0 {
        return vec4<f32>(0.0, 0.0, 0.0, 1.0);
    }
    let base = smooth_pixel(t_diffuse, s_diffuse, crt_uv, textureDimensions(t_diffuse), .40);
    var color = crt_noise(in.uv, 0.1) * 0.1 + base;
    return color;
}