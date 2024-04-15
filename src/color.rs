#[cfg(target_arch = "wasm32")]
use wasm_bindgen::prelude::*;

/// Represents a color in the RGB color space.
#[cfg_attr(target_arch = "wasm32", wasm_bindgen)]
#[derive(Debug, Clone, Copy)]
pub struct Color {
    pub r: u8,
    pub g: u8,
    pub b: u8,
}

impl Color {
    pub const BLACK: Self = Self::new(0, 0, 0);
    pub const WHITE: Self = Self::new(255, 255, 255);
    pub const RED: Self = Self::new(255, 0, 0);
    pub const GREEN: Self = Self::new(0, 255, 0);
    pub const BLUE: Self = Self::new(0, 0, 255);
    pub const YELLOW: Self = Self::new(255, 255, 0);
    pub const CYAN: Self = Self::new(0, 255, 255);
    pub const MAGENTA: Self = Self::new(255, 0, 255);

    /// Create a new color from the RGB values.
    pub const fn new(r: u8, g: u8, b: u8) -> Self {
        Self { r, g, b }
    }

    /// Create a new color from the hex value.
    pub const fn from_hex(hex: u32) -> Self {
        Self {
            r: ((hex >> 16) & 0xFF) as u8,
            g: ((hex >> 8) & 0xFF) as u8,
            b: (hex & 0xFF) as u8,
        }
    }

    /// Convert the color to a hex value.
    pub const fn to_hex(&self) -> u32 {
        ((self.r as u32) << 16) | ((self.g as u32) << 8) | self.b as u32
    }

    /// Convert the color to a hex string.
    pub fn to_hex_str(&self) -> String {
        format!("#{:02X}{:02X}{:02X}", self.r, self.g, self.b)
    }

    /// Create a color from a hex string.
    pub fn from_hex_str(hex: &str) -> Option<Self> {
        let hex = hex.trim_start_matches('#');
        let hex_value = u32::from_str_radix(hex, 16).ok()?;
        Some(Self::from_hex(hex_value))
    }

    /// Convert to linear color.
    /// Adapted from: https://github.com/three-rs/three/blob/07e47da5e0673aa9a16526719e16debd59040eec/src/color.rs#L39
    pub fn to_linear(&self) -> (f32, f32, f32) {
        let f = |xu: u8| {
            let x = (xu & 0xFF) as f32 / 255.0;
            if x > 0.04045 {
                ((x + 0.055) / 1.055).powf(2.4)
            } else {
                x / 12.92
            }
        };
        (f(self.r), f(self.g), f(self.b))
    }
}

impl Into<[f32; 3]> for Color {
    fn into(self) -> [f32; 3] {
        [
            self.r as f32 / 255.0,
            self.g as f32 / 255.0,
            self.b as f32 / 255.0,
        ]
    }
}

impl Into<glam::Vec3> for Color {
    fn into(self) -> glam::Vec3 {
        glam::Vec3::new(
            self.r as f32 / 255.0,
            self.g as f32 / 255.0,
            self.b as f32 / 255.0,
        )
    }
}
