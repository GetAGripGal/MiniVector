use encase::ShaderType;

/// The colors of the display
#[derive(Debug, ShaderType, Clone, Copy)]
pub struct DisplayColors {
    /// The background color
    pub primary: glam::Vec3,
    /// The electron color
    pub secondary: glam::Vec3,
}
