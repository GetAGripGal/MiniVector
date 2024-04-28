use std::collections::VecDeque;

use crate::{
    gfx::point::Point,
    instruction::{Instruction, InstructionKind},
};

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
        } else {
            self.instruction_buffer.len()
        };
        self.points.clear();
        for _ in 0..n_to_process {
            // If the instruction buffer is empty, break
            if self.instruction_buffer.is_empty() {
                break;
            }

            let instruction = self.instruction_buffer[0].clone();
            self.process_instruction(instruction);
            // Remove the first instruction
            self.instruction_buffer.pop_front();
        }
    }

    /// Return the points
    pub fn points(&self) -> &[Point] {
        &self.points
    }

    /// Process a single instruction and return the point the gun has moved to.
    /// Return true if a point was processed, false otherwise.
    fn process_instruction(&mut self, instruction: Instruction) -> bool {
        match instruction.kind {
            InstructionKind::Clear => {
                self.points.clear();
                self.should_clear = true;
            }
            InstructionKind::MoveTo => {
                let mut point = Point::from(instruction.data);
                point.power = if self.power { 1 } else { 0 };
                self.points.push_back(point);
                return true;
            }
            InstructionKind::PowerOn => {
                self.power = true;
            }
            InstructionKind::PowerOff => {
                self.power = false;
            }
        }
        false
    }

    /// Return if the renderer should be cleared. If so, return true and set the flag to false.
    pub fn should_clear(&mut self) -> bool {
        if self.should_clear {
            self.should_clear = false;
            true
        } else {
            false
        }
    }
}
