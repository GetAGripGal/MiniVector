#include "pipe.h"
#include "legacy/display.h"
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
    pipe->instruction_buffer = NULL;
    pipe->instruction_count = 0;
    pipe->buffered_bytes_count = 0;

    // Create the pipe
    mkfifo(pipe->pipe_path, 0666);
    pipe->fd = open(pipe->pipe_path, O_RDWR | O_NONBLOCK);

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
 * @return The instruction read from the pipe
 */
mv_instruction_t *mv_poll_pipe(mv_pipe *pipe, uint64_t instructions_per_frame)
{
    uint64_t instructions_buffer_size = 5;
    const int8_t instructions_buffer_increment = 10;
    int8_t bytes_read = 0;

    int8_t any_read = 0;

    int8_t buffer[BYTE_BUFFER_SIZE];

    do
    {
        any_read = 0;
        if (bytes_read = read(pipe->fd, buffer, sizeof(uint8_t)) > 0)
        {
            for (int8_t i = 0; i < bytes_read; i++)
            {
                if (pipe->buffered_bytes_count >= BYTE_BUFFER_SIZE)
                {
                    ERROR("Byte buffer overflow\n");
                    continue;
                }
                pipe->buffered_bytes[pipe->buffered_bytes_count] = buffer[i];
                pipe->buffered_bytes_count++;
            }
            any_read = 1;
        }
        TRACE("Read %d bytes\n", bytes_read);

        // If the amount of bytes in the buffer is big enough to contain an instruction
        // Read the instruction, and remove it from the buffer
        while (pipe->buffered_bytes_count >= MV_INSTRUCTION_SIZE)
        {
            // Reallocation of the instruction buffer
            pipe->instruction_buffer = realloc(pipe->instruction_buffer, (pipe->instruction_count + 1) * sizeof(mv_instruction_t));

            mv_instruction_t instruction = (mv_instruction_t){
                .type = pipe->buffered_bytes[0],
                .data = (pipe->buffered_bytes[1] << 24) | (pipe->buffered_bytes[2] << 16) | (pipe->buffered_bytes[3] << 8) | pipe->buffered_bytes[4]};

            pipe->instruction_buffer[pipe->instruction_count++] = instruction;

            TRACE("Read instruction(%d): %d, %d\n", pipe->instruction_count, instruction.type, instruction.data);
            // and move the rest of the buffer to the start
            for (int8_t i = 0; i < pipe->buffered_bytes_count - MV_INSTRUCTION_SIZE; i++)
            {
                if (i + MV_INSTRUCTION_SIZE < BYTE_BUFFER_SIZE)
                {
                    continue;
                }
                pipe->buffered_bytes[i] = pipe->buffered_bytes[i + MV_INSTRUCTION_SIZE];
            }
            pipe->buffered_bytes_count -= MV_INSTRUCTION_SIZE;
        }
    } while (instructions_per_frame > pipe->instruction_count && any_read);

    TRACE("Index: %d, Count: %d\n", pipe->index, pipe->instruction_count);

    if (pipe->instruction_count == 0)
    {
        return NULL;
    }

    if (pipe->index >= pipe->instruction_count)
    {
        pipe->instruction_count = 0;
        pipe->index = 0;
        return NULL;
    }

    return &pipe->instruction_buffer[pipe->index++];
}

/**
 * @brief Read an instruction from the pipe and remove it from the pipe
 *
 * @param pipe Pipe to read from
 */
mv_instruction_t *mv_pipe_read_instruction(mv_pipe *pipe)
{
    WARN("mv_pipe_read_instruction is deprecated\n");
    return NULL;
}