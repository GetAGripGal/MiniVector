use self::pipe::NamedPipeWriter;
use super::Event;
use crate::result::Result;

pub mod pipe;

/// Handles writing events
pub enum EventWriter {
    NamedPipe(NamedPipeWriter),
}

impl EventWriter {
    /// Write an event to the writer
    pub fn write(&mut self, events: &[Event]) -> Result<()> {
        match self {
            Self::NamedPipe(writer) => writer.write(events),
        }
    }
}

impl PartialEq for EventWriter {
    fn eq(&self, other: &Self) -> bool {
        match (self, other) {
            (Self::NamedPipe(_), Self::NamedPipe(_)) => true,
        }
    }
}
