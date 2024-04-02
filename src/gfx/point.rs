use encase::ShaderType;

/// A point drawn by the electron gun
#[derive(Debug, ShaderType, Clone, Copy)]
pub struct Point {
    pub x: f32,
    pub y: f32,
    pub power: u32,
}

impl Point {
    /// Create a new point
    pub fn new(x: f32, y: f32, power: bool) -> Self {
        Self {
            x,
            y,
            power: if power { 1 } else { 0 },
        }
    }

    /// Encode the point into a u32
    pub fn encode(&self) -> u32 {
        ((self.x as u32) << 16) | (self.y as u32)
    }
}

impl From<u32> for Point {
    fn from(data: u32) -> Self {
        let x = ((data >> 16) & 0xFFFF) as f32;
        let y = (data & 0xFFFF) as f32;
        Self { x, y, power: 0 }
    }
}
