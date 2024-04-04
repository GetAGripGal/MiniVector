use self::pipe::NamedPipeReader;

pub mod pipe;

/// Instruction readers provide a way to interface with minivector from different sources
#[derive(Debug)]
pub enum InstructionReader {
    None,
    NamedPipe(NamedPipeReader),
}

impl PartialEq for InstructionReader {
    fn eq(&self, other: &Self) -> bool {
        match (self, other) {
            (Self::None, Self::None) => true,
            (Self::NamedPipe(_), Self::NamedPipe(_)) => true,
            _ => false,
        }
    }
}
