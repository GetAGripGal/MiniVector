use crate::result::Result;

/// The size of an instruction in bytes
pub const INSTRUCTION_SIZE: usize = 5;

/// The type of an instruction
#[derive(Debug, Clone, Copy)]
pub enum InstructionKind {
    Clear,
    MoveTo,
    PowerOff,
    PowerOn,
}

/// An instruction executed by the renderer
#[derive(Debug, Clone, Copy)]
pub struct Instruction {
    pub kind: InstructionKind,
    pub data: u32,
}

impl Instruction {
    /// Create a new instruction
    pub fn new(type_id: InstructionKind, data: u32) -> Self {
        Self {
            kind: type_id,
            data,
        }
    }

    /// Get the data of the instruction
    pub fn get_data(&self) -> u32 {
        self.data
    }

    /// Parse a byte slice into an instruction
    pub fn from_bytes(bytes: [u8; INSTRUCTION_SIZE]) -> Result<Self> {
        let type_id = bytes[0];
        let type_id = InstructionKind::try_from(type_id)?;
        let data = (u32::from(bytes[1]) << 24)
            | (u32::from(bytes[2]) << 16)
            | (u32::from(bytes[3]) << 8)
            | u32::from(bytes[4]);
        Ok(Self::new(type_id, data))
    }

    /// Parse bytes into a vector of instructions
    pub fn from_bytes_slice(bytes: &[u8]) -> Result<Vec<Self>> {
        let mut instructions = Vec::new();
        for chunk in bytes.chunks(INSTRUCTION_SIZE) {
            if chunk.len() != INSTRUCTION_SIZE {
                log::error!("Bytes provided are not a multiple of 5, this might mean the file is corrupted.");
                break;
            }

            let mut bytes = [0; INSTRUCTION_SIZE];
            // Log chunk as hex
            bytes.copy_from_slice(chunk);
            match Instruction::from_bytes(bytes) {
                Ok(instruction) => instructions.push(instruction),
                Err(e) => log::warn!("Failed to parse instruction: {:?}", e),
            }
        }
        Ok(instructions)
    }
}

impl TryFrom<u8> for InstructionKind {
    type Error = anyhow::Error;

    fn try_from(value: u8) -> std::result::Result<Self, Self::Error> {
        match value {
            0 => Ok(Self::Clear),
            1 => Ok(Self::MoveTo),
            2 => Ok(Self::PowerOff),
            3 => Ok(Self::PowerOn),
            _ => Err(anyhow::anyhow!("Invalid instruction kind: {}", value)),
        }
    }
}
