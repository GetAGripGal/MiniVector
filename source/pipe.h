#pragma once

#include "instruction.h"

#include <stdio.h>

/**
 * @brief Pipe structure
 */
typedef struct
{
    FILE *fd;
    char *pipe_path;
    uint32_t index; // Points to where the next instruction starts
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
 * @param display Display to operate on
 */
void mv_poll_pipe(mv_pipe *pipe, mv_display *display, mv_frame *frame, mv_renderer *renderer);

/**
 * @brief Read an instruction from the pipe
 *
 * @param pipe Pipe to read from
 */
mv_instruction_t *mv_pipe_read_instruction(mv_pipe *pipe);