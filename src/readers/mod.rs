use self::pipe::NamedPipeReader;
use crate::result::Result;

pub mod pipe;

/// Instruction readers provide a way to interface with minivector from different sources
#[derive(Debug)]
pub enum InstructionReader {
    NamedPipe(NamedPipeReader),
}

impl InstructionReader {
    /// Read an instruction from the reader
    pub fn read(&mut self) -> Result<Vec<u8>> {
        match self {
            Self::NamedPipe(reader) => reader.read(),
        }
    }
}

impl PartialEq for InstructionReader {
    fn eq(&self, other: &Self) -> bool {
        match (self, other) {
            (Self::NamedPipe(_), Self::NamedPipe(_)) => true,
        }
    }
}
