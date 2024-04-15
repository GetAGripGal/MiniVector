pub mod color;
pub mod config;
pub mod context;
pub mod events;
pub mod gfx;
pub mod instruction;
pub mod processor;
pub mod readers;
pub mod result;
pub mod timing;

pub mod prelude {
    pub use crate::color::Color;
    pub use crate::config::Config;
    pub use crate::context::Context;
    pub use crate::gfx::electron::ElectronParams;
    pub use crate::instruction::Instruction;
    pub use crate::processor::InstructionProcessor;
    pub use crate::result::Result;
    pub use crate::timing::FrameTimer;
}
