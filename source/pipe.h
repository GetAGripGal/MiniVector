#pragma once

#include "instruction.h"

#include <stdio.h>

#define BYTE_BUFFER_SIZE 1024

/**
 * @brief Pipe structure
 */
typedef struct
{
    int32_t fd;
    char *pipe_path;
    uint64_t index;                           // Points to where the next instruction starts
    mv_instruction_t *instruction_buffer;     // Buffer for the instruction
    uint64_t instruction_count;               // Number of instructions in the buffer
    uint8_t buffered_bytes[BYTE_BUFFER_SIZE]; // Buffer for the bytes
    uint32_t buffered_bytes_count;            // Number of bytes in the buffer
} mv_pipe;

/**
 * @brief Create a pipe
 *
 * @param pipe_path Path to the pipe
 */
mv_pipe *mv_create_pipe(char *pipe_path);

/**
 * @brief Destroy a pipe
 *
 * @param pipe Pipe to destroy
 */
void mv_destroy_pipe(mv_pipe *pipe);

/**
 * @brief Poll a pipe
 *
 * @param pipe Pipe to poll
 * @return The instruction read from the pipe
 */
mv_instruction_t *mv_poll_pipe(mv_pipe *pipe);

/**
 * @brief Read an instruction from the pipe
 *
 * @param pipe Pipe to read from
 */
mv_instruction_t *mv_pipe_read_instruction(mv_pipe *pipe);