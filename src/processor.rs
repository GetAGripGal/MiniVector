use crate::{
    gfx::point::Point,
    instruction::{Instruction, InstructionKind},
};

use bytemuck::Contiguous;
use slice_deque::SliceDeque;
#[cfg(target_arch = "wasm32")]
use wasm_bindgen::prelude::*;

/// Processes the instructions
#[cfg_attr(target_arch = "wasm32", wasm_bindgen)]
pub struct InstructionProcessor {
    // The instruction buffer
    instruction_buffer: SliceDeque<Instruction>,
    points: SliceDeque<Point>,
    power: bool,
    should_clear: bool,
    instructions_per_frame: usize,
}

impl InstructionProcessor {
    /// Create a new instruction processor
    pub fn new(instructions_per_frame: usize) -> Self {
        Self {
            instruction_buffer: SliceDeque::new(),
            points: SliceDeque::with_capacity(instructions_per_frame),
            instructions_per_frame,
            power: true,
            should_clear: false,
        }
    }

    /// Push instructions to the instruction buffer
    pub fn push_instructions(&mut self, instructions: &[Instruction]) {
        self.instruction_buffer.extend(instructions);
    }

    /// Process a set of instructions
    pub fn process(&mut self) {
        let n_to_process = if self.instructions_per_frame > 0 {
            self.instructions_per_frame
                .min(self.instruction_buffer.len())
        } else {
            self.instruction_buffer.len()
        };

        // Batch processing of instructions
        let mut power = self.power;
        let mut points = SliceDeque::new();
        for instruction in self.instruction_buffer.drain(..n_to_process) {
            Self::process_instruction(instruction, &mut power, &mut points);
        }
        // Append the previous last point to the new points list
        if let Some(last_point) = self.points.last() {
            points.push_front(*last_point);
        }
        self.points = points;
        self.power = power;
    }

    /// Return the points
    pub fn points(&self) -> &[Point] {
        &self.points
    }

    /// Process a single instruction and return the point the gun has moved to.
    /// Return true if a point was processed, false otherwise.
    fn process_instruction(
        instruction: Instruction,
        power: &mut bool,
        points: &mut SliceDeque<Point>,
    ) -> bool {
        match instruction.kind {
            InstructionKind::Clear => {
                // self.should_clear = true;
            }
            InstructionKind::MoveTo => {
                let mut point = Point::from(instruction.data);
                point.power = power.into_integer() as u32;
                points.push_back(point);
                return true;
            }
            InstructionKind::PowerOn => {
                *power = true;
            }
            InstructionKind::PowerOff => {
                *power = false;
            }
        }
        false
    }

    /// Return if the renderer should be cleared. If so, return true and set the flag to false.
    pub fn should_clear(&mut self) -> bool {
        self.should_clear
            .then(|| {
                self.should_clear = false;
                true
            })
            .unwrap_or(false)
    }
}
