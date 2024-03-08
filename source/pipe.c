#include "pipe.h"
#include "display.h"
#include "log.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Create a pipe
 *
 * @param pipe_path Path to the pipe
 */
mv_pipe *mv_create_pipe(char *pipe_path)
{
    mv_pipe *pipe = malloc(sizeof(mv_pipe));
    pipe->pipe_path = pipe_path;
    pipe->index = 0;

    // Clear the pipe
    remove(pipe->pipe_path);
    return pipe;
}

/**
 * @brief Destroy a pipe
 *
 * @param pipe Pipe to destroy
 */
void mv_destroy_pipe(mv_pipe *pipe)
{
    TRACE("Destroying pipe %s\n", pipe->pipe_path);
    // delete the pipe
    remove(pipe->pipe_path);
    free(pipe);
}

/**
 * @brief Poll a pipe
 *
 * @param pipe Pipe to poll
 * @param display Display to operate on
 * @param frame Frame to operate on
 * @param renderer Renderer to operate on
 */
void mv_poll_pipe(mv_pipe *pipe, mv_display *display, mv_frame *frame, mv_renderer *renderer)
{
    pipe->fd = fopen(pipe->pipe_path, "r");

    if (!pipe->fd)
    {
        pipe->fd = fopen(pipe->pipe_path, "w");
        if (!pipe->fd)
        {
            ERROR("Unable to open pipe %s\n", pipe->pipe_path);
            exit(1);
        }
        fclose(pipe->fd);
        pipe->fd = fopen(pipe->pipe_path, "r");
    }

    mv_instruction_t *instruction = mv_pipe_read_instruction(pipe);
    if (instruction)
    {
        mv_process_instruction(*instruction, display, frame, renderer);
        free(instruction);
    }
    fclose(pipe->fd);
}

/**
 * @brief Read an instruction from the pipe and remove it from the pipe
 *
 * @param pipe Pipe to read from
 */
mv_instruction_t *mv_pipe_read_instruction(mv_pipe *pipe)
{
    // The index points to the start of the next instruction
    // If the file is bigger than the index, there is a new instruction
    uint32_t file_size = 0;
    fseek(pipe->fd, 0L, SEEK_END);
    file_size = ftell(pipe->fd);
    fseek(pipe->fd, 0L, SEEK_END);
    if (file_size < pipe->index * MV_INSTRUCTION_SIZE)
    {
        TRACE("No new instruction in the pipe. File size is: %u, required: %u\n", file_size, pipe->index * sizeof(mv_instruction_t));
        return NULL;
    }

    // Skip if the instruction is not ready
    if (file_size < (pipe->index + 1) * MV_INSTRUCTION_SIZE)
    {
        return NULL;
    }

    // Read the latest instruction
    uint8_t *buffer = malloc(MV_INSTRUCTION_SIZE);
    fseek(pipe->fd, pipe->index * MV_INSTRUCTION_SIZE, SEEK_SET);
    fread(buffer, MV_INSTRUCTION_SIZE, 1, pipe->fd);

    // Convert the buffer to an instruction
    mv_instruction_t *instruction = malloc(sizeof(mv_instruction_t));
    *instruction = mv_read_instruction(buffer);
    free(buffer);

    // Log the buffer as a single hex
    TRACE("Read instruction: 0x%02X%02X%02X%02X%02X\n", buffer[0], buffer[1], buffer[2], buffer[3], buffer[4]);

    // Increment the index
    pipe->index++;

    return instruction;
}