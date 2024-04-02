use crate::color::Color;

#[cfg(target_arch = "wasm32")]
use wasm_bindgen::prelude::*;

const DEFAULT_WINDOW_WIDTH: u32 = 1280;
const DEFAILT_WINDOW_HEIGHT: u32 = 720;

const DEFAULT_RESOLUTION_WIDTH: u32 = 1920;
const DEFAULT_RESOLUTION_HEIGHT: u32 = 1080;

const DEFAULT_INSTRUCTION_PER_FRAME: u32 = 800;
const DEFAULT_FRAME_RATE: u32 = 60;

const DEFAULT_PRIMARY_COLOR: Color = Color::new(40, 40, 40);
const DEFAULT_SECONDARY_COLOR: Color = Color::new(51, 255, 100);

const DEFAULT_RADIUS: f32 = 1.0;
const DEFAULT_DIM_FACTOR: f32 = 0.3;

const MINIVECTOR_ICON: &'static [u8] = include_bytes!("../assets/icon.png");

#[cfg(target_os = "windows")]
const DEFAULT_PIPE: &'static str = "\\\\.\\pipe\\mv_pipe";
#[cfg(not(target_os = "windows"))]
const DEFAULT_PIPE: &'static str = "/tmp/mv_pipe";

/// The usage text
pub const USAGE_TEXT: &str = r"
usage: minivector [options]
    options:
        window:
          -w,  --window <width> <height>     Set the window siz
          -f   --fullscreen                  Set the window to fullscree
        display
          -r,  --resolution <width> <height> Set the resolutio
          -p,  --primary <color_hex>         Set the primary colo
          -s,  --secondary <color_hex>       Set the secondary colo
        gun
          -rg, --radius <radius>             Set the radius of the electron gun
          -df, --dim-factor <factor>         Set the dim factor per frame
        executor:
          -i,  --pipe <pipe>                 Set the pipe to read the instructions
          -e,  --instruction-per-frame <n>   Set the number of instructions per frame
          -fr, --frame-rate <n>              Set the frame rate
";

/// The resolution of the window
#[cfg_attr(target_arch = "wasm32", wasm_bindgen)]
#[derive(Debug, Clone, Copy)]
pub struct Resolution {
    pub width: u32,
    pub height: u32,
}

#[cfg_attr(target_arch = "wasm32", wasm_bindgen)]
impl Resolution {
    /// Create a new resolution
    #[cfg_attr(target_arch = "wasm32", wasm_bindgen(constructor))]
    pub fn new(width: u32, height: u32) -> Self {
        Self { width, height }
    }
}

/// The window icon
#[derive(Debug, Clone)]
pub struct Icon {
    pub width: u32,
    pub height: u32,
    pub bytes: Vec<u8>,
}

/// The minivector configuration
#[cfg(not(target_arch = "wasm32"))]
#[derive(Debug)]
pub struct Config {
    pub window: Resolution,
    pub fullscreen: bool,
    pub resolution: Resolution,
    pub primary: Color,
    pub secondary: Color,
    pub radius: f32,
    pub dim_factor: f32,
    pub pipe: String,
    pub instruction_per_frame: u32,
    pub frame_rate: u32,
    pub icon: Icon,
}

/// The minivector configuration
#[cfg(target_arch = "wasm32")]
#[wasm_bindgen]
#[derive(Debug)]
pub struct Config {
    pub(crate) window: Resolution,
    pub(crate) fullscreen: bool,
    pub(crate) resolution: Resolution,
    pub(crate) primary: Color,
    pub(crate) secondary: Color,
    pub(crate) radius: f32,
    pub(crate) dim_factor: f32,
    pub(crate) pipe: String,
    pub(crate) instruction_per_frame: u32,
    pub(crate) frame_rate: u32,
}

impl Default for Config {
    fn default() -> Self {
        Self {
            window: Resolution {
                width: DEFAULT_WINDOW_WIDTH,
                height: DEFAILT_WINDOW_HEIGHT,
            },
            fullscreen: false,
            resolution: Resolution {
                width: DEFAULT_RESOLUTION_WIDTH,
                height: DEFAULT_RESOLUTION_HEIGHT,
            },
            primary: DEFAULT_PRIMARY_COLOR,
            secondary: DEFAULT_SECONDARY_COLOR,
            radius: DEFAULT_RADIUS,
            dim_factor: DEFAULT_DIM_FACTOR,
            pipe: DEFAULT_PIPE.to_string(),
            instruction_per_frame: DEFAULT_INSTRUCTION_PER_FRAME,
            frame_rate: DEFAULT_FRAME_RATE,
            #[cfg(not(target_arch = "wasm32"))]
            icon: load_icon().unwrap(),
        }
    }
}

#[cfg_attr(target_arch = "wasm32", wasm_bindgen)]
impl Config {
    /// Create a new configuration
    #[cfg(target_arch = "wasm32")]
    #[wasm_bindgen(constructor)]
    pub fn new() -> Self {
        Self::default()
    }

    /// Getter for the window
    #[cfg(target_arch = "wasm32")]
    #[cfg_attr(target_arch = "wasm32", wasm_bindgen(getter))]
    pub fn window(&self) -> Resolution {
        self.window
    }

    /// Setter for the window
    #[cfg(target_arch = "wasm32")]
    #[cfg_attr(target_arch = "wasm32", wasm_bindgen(setter))]
    pub fn set_window(&mut self, value: Resolution) {
        self.window = value;
    }

    /// Getter for the fullscreen
    #[cfg(target_arch = "wasm32")]
    #[cfg_attr(target_arch = "wasm32", wasm_bindgen(getter))]
    pub fn fullscreen(&self) -> bool {
        self.fullscreen
    }

    /// Setter for the fullscreen
    #[cfg(target_arch = "wasm32")]
    #[cfg_attr(target_arch = "wasm32", wasm_bindgen(setter))]
    pub fn set_fullscreen(&mut self, value: bool) {
        self.fullscreen = value;
    }

    /// Getter for the resolution
    #[cfg(target_arch = "wasm32")]
    #[cfg_attr(target_arch = "wasm32", wasm_bindgen(getter))]
    pub fn resolution(&self) -> Resolution {
        self.resolution
    }

    /// Setter for the resolution
    #[cfg(target_arch = "wasm32")]
    #[cfg_attr(target_arch = "wasm32", wasm_bindgen(setter))]
    pub fn set_resolution(&mut self, value: Resolution) {
        self.resolution = value;
    }

    /// Getter for the primary color
    #[cfg(target_arch = "wasm32")]
    #[cfg_attr(target_arch = "wasm32", wasm_bindgen(getter))]
    pub fn primary(&self) -> String {
        self.primary.to_hex_str()
    }

    /// Setter for the primary color
    #[cfg(target_arch = "wasm32")]
    #[cfg_attr(target_arch = "wasm32", wasm_bindgen(setter))]
    pub fn set_primary(&mut self, value: &str) {
        self.primary = Color::from_hex_str(value).unwrap();
    }

    /// Getter for the secondary color
    #[cfg(target_arch = "wasm32")]
    #[cfg_attr(target_arch = "wasm32", wasm_bindgen(getter))]
    pub fn secondary(&self) -> String {
        self.secondary.to_hex_str()
    }

    /// Setter for the secondary color
    #[cfg(target_arch = "wasm32")]
    #[cfg_attr(target_arch = "wasm32", wasm_bindgen(setter))]
    pub fn set_secondary(&mut self, value: &str) {
        self.secondary = Color::from_hex_str(value).unwrap();
    }

    /// Getter for the radius
    #[cfg(target_arch = "wasm32")]
    #[cfg_attr(target_arch = "wasm32", wasm_bindgen(getter))]
    pub fn radius(&self) -> f32 {
        self.radius
    }

    /// Setter for the radius
    #[cfg(target_arch = "wasm32")]
    #[cfg_attr(target_arch = "wasm32", wasm_bindgen(setter))]
    pub fn set_radius(&mut self, value: f32) {
        self.radius = value;
    }

    /// Getter for the dim factor
    #[cfg(target_arch = "wasm32")]
    #[cfg_attr(target_arch = "wasm32", wasm_bindgen(getter))]
    pub fn dim_factor(&self) -> f32 {
        self.dim_factor
    }

    /// Setter for the dim factor
    #[cfg(target_arch = "wasm32")]
    #[cfg_attr(target_arch = "wasm32", wasm_bindgen(setter))]
    pub fn set_dim_factor(&mut self, value: f32) {
        self.dim_factor = value;
    }

    /// Getter for the instruction per frame
    #[cfg(target_arch = "wasm32")]
    #[cfg_attr(target_arch = "wasm32", wasm_bindgen(getter))]
    pub fn instruction_per_frame(&self) -> u32 {
        self.instruction_per_frame
    }

    /// Setter for the instruction per frame
    #[cfg(target_arch = "wasm32")]
    #[cfg_attr(target_arch = "wasm32", wasm_bindgen(setter))]
    pub fn set_instruction_per_frame(&mut self, value: u32) {
        self.instruction_per_frame = value;
    }

    /// Getter for the frame rate
    #[cfg(target_arch = "wasm32")]
    #[cfg_attr(target_arch = "wasm32", wasm_bindgen(getter))]
    pub fn frame_rate(&self) -> u32 {
        self.frame_rate
    }

    /// Getter for the pipe
    #[cfg(target_arch = "wasm32")]
    #[cfg_attr(target_arch = "wasm32", wasm_bindgen(getter))]
    pub fn pipe(&self) -> String {
        self.pipe.clone()
    }

    /// Setter for the pipe
    #[cfg(target_arch = "wasm32")]
    #[cfg_attr(target_arch = "wasm32", wasm_bindgen(setter))]
    pub fn set_pipe(&mut self, value: &str) {
        self.pipe = value.to_string();
    }
}

/// Loading the icon from the embedded bytes
#[cfg(not(target_arch = "wasm32"))]
fn load_icon() -> anyhow::Result<Icon> {
    let img = image::load_from_memory(MINIVECTOR_ICON)?;
    let img = img.into_rgba8();
    let dimension = img.dimensions();
    let rgba = img.into_raw();
    Ok(Icon {
        width: dimension.0,
        height: dimension.1,
        bytes: rgba.to_vec(),
    })
}
