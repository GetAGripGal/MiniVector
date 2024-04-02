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

/// Curve the screen like a crt display
fn crt_curve(uv: vec2<f32>) -> vec2<f32> {
    var crt_uv: vec2<f32> = uv * 2.0 - 1.0;
    let offset: vec2<f32> = abs(crt_uv.yx) / vec2<f32>(6.0, 4.0);
    crt_uv = crt_uv + crt_uv * offset * offset;
    crt_uv = crt_uv * 0.5 + 0.5;
    return crt_uv;
}

/// The crt vingette effect
fn crt_vignette(uv: vec2<f32>) -> vec4<f32> {
    let vignette = uv.x * uv.y * (1.0 - uv.x) * (1.0 - uv.y);
    return vec4(vignette, vignette, vignette, 1.0);
}

@fragment
fn fragment(in: VertexOutput) -> @location(0) vec4<f32> {
    let crt_uv = crt_curve(in.uv);
    if crt_uv.x < 0.0 || crt_uv.x > 1.0 || crt_uv.y < 0.0 || crt_uv.y > 1.0 {
        return vec4<f32>(0.0, 0.0, 0.0, 1.0);
    }
    let base = textureSample(t_diffuse, s_diffuse, crt_uv);
    var color = base;
    color.r = clamp(color.r * 3.0, 0.0, 1.0);
    color.g = clamp(color.g * 3.0, 0.0, 1.0);
    color.b = clamp(color.b * 3.0, 0.0, 1.0);
    return color;
}