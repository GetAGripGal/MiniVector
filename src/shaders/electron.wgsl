/// The colors used to display the output of the shader.
struct DisplayColors {
    primary: vec3<f32>,
    secondary: vec3<f32>,
}

/// A point of the electron gun
struct Point {
    x: f32,
    y: f32,
    power: u32,
};

/// A buffer of points of the electron gun
struct PointBuffer {
    points: array<Point>,
};

/// The parameters of the shader
struct Parameters {
    radius: f32,
    dim_factor: f32,
};

@group(0) @binding(0) 
var output_texture: texture_storage_2d<rgba32float, write>;
@group(0) @binding(1)
var t_diffuse: texture_2d<f32>;

@group(1) @binding(0)
var<storage, read> display_colors: DisplayColors;

@group(2) @binding(0)
var<storage, read> point_buffer: PointBuffer;
@group(2) @binding(1)
var<storage, read> points_amount: u32;

@group(3) @binding(0)
var<uniform> parameters: Parameters;

/// Check if a point is between two other points
fn is_point_between_two_points(point: vec2<f32>, a: vec2<f32>, b: vec2<f32>, radius: f32) -> bool {
    let ap: vec2<f32> = point - a;
    let ab: vec2<f32> = b - a;

    // Calculate square of length of ab
    let ab_lenght_sqrd: f32 = dot(ab, ab);

    // If abLengthSq is 0, a and b are the same point, return false
    if ab_lenght_sqrd == 0.0 {
        return false;
    }

    // Calculate parameter t along ab, clamped between 0 and 1
    let t: f32 = clamp(dot(ap, ab) / ab_lenght_sqrd, 0.0, 1.0);

    // Calculate closest point on ab to the point
    let closest: vec2<f32> = a + t * ab;

    // Calculate distance between closest point and the point
    let dist: f32 = distance(point, closest);

    // Check if distance is within radius
    return dist <= radius;
}

/// Check if a pixel should be drawn
fn should_draw(pixel_coords: vec2<f32>, electron_gun_radius: f32) -> bool {
    // If the point is in between the point behind it reached this frame, draw it
    for (var i: u32 = 0u; i < points_amount - 1u; i = i + 1u) {
        let point = point_buffer.points[i];
        let next_point = point_buffer.points[i + 1u];
        if is_point_between_two_points(pixel_coords, vec2<f32>(point.x, point.y), vec2<f32>(next_point.x, next_point.y), electron_gun_radius) {
            if point.power != 0u && next_point.power != 0u {
                return true;
            }
        }
    }
    return false;
}

/// Convert a color from sRGB to HSV
fn srgb_to_hsv(color: vec3<f32>) -> vec3<f32> {
    let min_val = min(min(color.r, color.g), color.b);
    let max_val = max(max(color.r, color.g), color.b);
    let delta = max_val - min_val;

    var hsv: vec3<f32> = vec3<f32>(0.0, 0.0, max_val);

    if delta > 0.00001 {
        if max_val != 0.0 {
            hsv.g = delta / max_val;
        } else {
            hsv.g = 0.0;
            hsv.r = 0.0;
            return hsv;
        }

        if color.r == max_val {
            hsv.r = (color.g - color.b) / delta;
        } else if color.g == max_val {
            hsv.r = 2.0 + (color.b - color.r) / delta;
        } else {
            hsv.r = 4.0 + (color.r - color.g) / delta;
        }

        hsv.r *= 60.0;

        if hsv.r < 0.0 {
            hsv.r += 360.0;
        }
    } else {
        hsv.r = 0.0;
        hsv.g = 0.0;
    }

    return hsv;
}

/// Convert a color from HSV to sRGB
fn hsv_to_srgb(hsv: vec3<f32>) -> vec3<f32> {
    var h = hsv.r;
    var s = hsv.g;
    var v = hsv.b;

    if s <= 0.0 {
        return vec3<f32>(v, v, v);
    }

    h = h / 60.0;
    let i = floor(h);
    let f = h - i;
    let p = v * (1.0 - s);
    let q = v * (1.0 - s * f);
    let t = v * (1.0 - s * (1.0 - f));

    if i == 0.0 {
        return vec3<f32>(v, t, p);
    } else if i == 1.0 {
        return vec3<f32>(q, v, p);
    } else if i == 2.0 {
        return vec3<f32>(p, v, t);
    } else if i == 3.0 {
        return vec3<f32>(p, q, v);
    } else if i == 4.0 {
        return vec3<f32>(t, p, v);
    } else {
        return vec3<f32>(v, p, q);
    }
}

/// Dim an sRGB color
fn dim_srgb(color: vec3<f32>, amount: f32) -> vec3<f32> {
    var hsv = srgb_to_hsv(color);
    hsv.b = hsv.b * amount;
    return hsv_to_srgb(hsv);
}

@compute
@workgroup_size(8, 8, 1)
fn main(
    @builtin(global_invocation_id) gid: vec3<u32>
) {
    let dimensions = textureDimensions(t_diffuse);
    let coords = vec2<i32>(gid.xy);
    let uv = vec2<f32>(coords) / vec2<f32>(dimensions);

    let dim = 1.0 - parameters.dim_factor;

    var color = textureLoad(t_diffuse, coords, 0);
    color = vec4<f32>(dim_srgb(color.rgb, dim), color.a);
    color.a = color.a * dim;

    if should_draw(vec2<f32>(coords), parameters.radius) {
        color = vec4<f32>(display_colors.secondary, 1.0);
    }
    textureStore(output_texture, coords, color);
}