use self::{keycode::KeyCode, mouse::MouseButton};
use crate::result::Result;

pub mod keycode;
pub mod manager;
pub mod mouse;
pub mod writer;

/// The different types of events that can be dispatched by minivector
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum EventKind {
    FrameFinished,
    KeyPressed,
    KeyReleased,
    MouseMoved,
    MousePressed,
    MouseReleased,
    InstructionBufferEmpty,
}

/// An event that gets dispatched by minivector.
/// This allows other applications to interact with minivector
#[derive(Debug, PartialEq, Eq, Clone, Copy)]
pub struct Event {
    /// The event kind
    pub kind: EventKind,
    /// The event data
    pub data: u64,
}

impl Event {
    /// Create a new event
    pub fn new(kind: EventKind, data: u64) -> Self {
        Self { kind, data }
    }

    /// Create a new key pressed event
    pub fn key_pressed(code: KeyCode) -> Self {
        Self::new(EventKind::KeyPressed, code.into())
    }

    /// Create a new key released event
    pub fn key_released(code: KeyCode) -> Self {
        Self::new(EventKind::KeyReleased, code.into())
    }

    /// Create a new mouse moved event
    pub fn mouse_moved(x: u32, y: u32) -> Self {
        Self::new(EventKind::MouseMoved, (u64::from(x) << 32) | u64::from(y))
    }

    /// Create a new mouse pressed event
    pub fn mouse_pressed(button: MouseButton) -> Self {
        Self::new(EventKind::MousePressed, button.into())
    }

    /// Create a new mouse released event
    pub fn mouse_released(button: MouseButton) -> Self {
        Self::new(EventKind::MouseReleased, button.into())
    }

    // Create a new frame finished event
    pub fn frame_finished() -> Self {
        Self::new(EventKind::FrameFinished, 0)
    }

    /// Get the key code of the event
    pub fn key_code(&self) -> Result<KeyCode> {
        match self.kind {
            EventKind::KeyPressed | EventKind::KeyReleased => Ok(self.data.into()),
            _ => Err(anyhow::anyhow!("Event is not a key event").into()),
        }
    }

    /// Get the mouse position of the event
    pub fn mouse_position(&self) -> Result<(u32, u32)> {
        match self.kind {
            EventKind::MouseMoved => {
                let x = (self.data >> 32) as u32;
                let y = self.data as u32;
                Ok((x, y))
            }
            _ => Err(anyhow::anyhow!("Event is not a mouse moved event").into()),
        }
    }

    /// Parse an event from bytes
    pub fn from_bytes(bytes: [u8; 9]) -> Result<Self> {
        let kind = match bytes[0] {
            0 => EventKind::FrameFinished,
            1 => EventKind::KeyPressed,
            2 => EventKind::KeyReleased,
            3 => EventKind::MouseMoved,
            4 => EventKind::MousePressed,
            5 => EventKind::MouseReleased,
            6 => EventKind::InstructionBufferEmpty,
            _ => return Err(anyhow::anyhow!("Invalid event kind").into()),
        };

        let data = u64::from_be_bytes([
            bytes[1], bytes[2], bytes[3], bytes[4], bytes[5], bytes[6], bytes[7], bytes[8],
        ]);
        Ok(Self::new(kind, data))
    }

    /// Convert an event to bytes
    pub fn to_bytes(&self) -> [u8; 9] {
        let kind: u8 = self.kind.into();
        let data = self.data.to_be_bytes();

        let mut bytes = [0; 9];
        bytes[0] = kind;
        bytes[1..].copy_from_slice(&data);
        bytes
    }
}

impl Into<u8> for EventKind {
    fn into(self) -> u8 {
        match self {
            EventKind::FrameFinished => 0,
            EventKind::KeyPressed => 1,
            EventKind::KeyReleased => 2,
            EventKind::MouseMoved => 3,
            EventKind::MousePressed => 4,
            EventKind::MouseReleased => 5,
            EventKind::InstructionBufferEmpty => 6,
        }
    }
}
