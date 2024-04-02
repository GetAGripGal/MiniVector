use minivector::{
    color::Color,
    config::{Config, Resolution},
    context::Context,
    result::Result,
};

/// The usage text
const USAGE_TEXT: &str = r"
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

/// The instructions to render
const INSTRUCTIONS: &'static [u8] = include_bytes!("../instructions.mv");

/// The error type for invalid arguments
#[derive(Debug)]
pub enum InvalidArgumentError {
    InvalidWindowSize,
    InvalidResolution,
    InvalidPrimaryColor,
    InvalidSecondaryColor,
    InvalidRadius,
    InvalidDimFactor,
    InvalidPipe,
    InvalidInstructionPerFrame,
    InvalidFrameRate,
}

fn main() -> Result<()> {
    let args: Vec<String> = std::env::args().collect();
    let config = read_args(args).unwrap_or_else(|e| {
        println!("{}", e);
        println!("{}", USAGE_TEXT);
        std::process::exit(1);
    });
    futures::executor::block_on(run(config))
}

/// Run minivector
async fn run(config: Config) -> Result<()> {
    let mut context = Context::new(config).await?;
    context.push_instructions(INSTRUCTIONS);
    context.run().await
}

/// Parse the command line arguments
pub fn read_args(args: Vec<String>) -> std::result::Result<Config, InvalidArgumentError> {
    let mut config = Config::default();
    let mut i = 1;

    while i < args.len() {
        match args[i].as_str() {
            "-w" | "--window" => {
                let width = args
                    .get(i + 1)
                    .ok_or(InvalidArgumentError::InvalidWindowSize)?;
                let height = args
                    .get(i + 2)
                    .ok_or(InvalidArgumentError::InvalidWindowSize)?;
                config.window = Resolution {
                    width: width
                        .parse()
                        .map_err(|_| InvalidArgumentError::InvalidWindowSize)?,
                    height: height
                        .parse()
                        .map_err(|_| InvalidArgumentError::InvalidWindowSize)?,
                };
                i += 3;
            }
            "-f" | "--fullscreen" => {
                config.fullscreen = true;
                i += 1;
            }
            "-r" | "--resolution" => {
                let width = args
                    .get(i + 1)
                    .ok_or(InvalidArgumentError::InvalidResolution)?;
                let height = args
                    .get(i + 2)
                    .ok_or(InvalidArgumentError::InvalidResolution)?;
                config.resolution = Resolution {
                    width: width
                        .parse()
                        .map_err(|_| InvalidArgumentError::InvalidResolution)?,
                    height: height
                        .parse()
                        .map_err(|_| InvalidArgumentError::InvalidResolution)?,
                };
                i += 3;
            }
            "-p" | "--primary" => {
                let value = args
                    .get(i + 1)
                    .ok_or(InvalidArgumentError::InvalidPrimaryColor)?;
                config.primary =
                    Color::from_hex_str(value).ok_or(InvalidArgumentError::InvalidPrimaryColor)?;
                i += 2;
            }
            "-s" | "--secondary" => {
                let value = args
                    .get(i + 1)
                    .ok_or(InvalidArgumentError::InvalidSecondaryColor)?;
                config.secondary = Color::from_hex_str(value)
                    .ok_or(InvalidArgumentError::InvalidSecondaryColor)?;
                i += 2;
            }
            "-rg" | "--radius" => {
                let value = args.get(i + 1).ok_or(InvalidArgumentError::InvalidRadius)?;
                config.radius = value
                    .parse()
                    .map_err(|_| InvalidArgumentError::InvalidRadius)?;
                i += 2;
            }
            "-df" | "--dim-factor" => {
                let value = args
                    .get(i + 1)
                    .ok_or(InvalidArgumentError::InvalidDimFactor)?;
                config.dim_factor = value
                    .parse()
                    .map_err(|_| InvalidArgumentError::InvalidDimFactor)?;
                i += 2;
            }
            "-i" | "--pipe" => {
                let value = args.get(i + 1).ok_or(InvalidArgumentError::InvalidPipe)?;
                config.pipe = value.to_string();
                i += 2;
            }
            "-e" | "--instruction-per-frame" => {
                let value = args
                    .get(i + 1)
                    .ok_or(InvalidArgumentError::InvalidInstructionPerFrame)?;
                config.instruction_per_frame = value
                    .parse()
                    .map_err(|_| InvalidArgumentError::InvalidInstructionPerFrame)?;
                i += 2;
            }
            "-fr" | "--frame-rate" => {
                let value = args
                    .get(i + 1)
                    .ok_or(InvalidArgumentError::InvalidFrameRate)?;
                config.frame_rate = value
                    .parse()
                    .map_err(|_| InvalidArgumentError::InvalidFrameRate)?;
                i += 2;
            }
            "-h" | "--help" => {
                println!("{}", USAGE_TEXT);
                std::process::exit(0);
            }
            _ => {
                i += 1;
            }
        }
    }

    Ok(config)
}

impl std::error::Error for InvalidArgumentError {}

impl std::fmt::Display for InvalidArgumentError {
    fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
        match self {
            InvalidArgumentError::InvalidWindowSize => write!(f, "Invalid window size"),
            InvalidArgumentError::InvalidResolution => write!(f, "Invalid resolution"),
            InvalidArgumentError::InvalidPrimaryColor => write!(f, "Invalid primary color"),
            InvalidArgumentError::InvalidSecondaryColor => write!(f, "Invalid secondary color"),
            InvalidArgumentError::InvalidRadius => write!(f, "Invalid radius"),
            InvalidArgumentError::InvalidDimFactor => write!(f, "Invalid dim factor"),
            InvalidArgumentError::InvalidPipe => write!(f, "Invalid pipe"),
            InvalidArgumentError::InvalidInstructionPerFrame => {
                write!(f, "Invalid instruction per frame")
            }
            InvalidArgumentError::InvalidFrameRate => write!(f, "Invalid frame rate"),
        }
    }
}
