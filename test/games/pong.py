import ctypes
from dataclasses import dataclass
from io import BufferedReader
import os
import math
import time

# The pipes
INSTRUCTION_PIPE = "/tmp/mv_pipe"
EVENT_PIPE = "/tmp/mv_pong_pipe"

# The size of an event in bytes
EVENT_SIZE = 9

# Define the event types
EVENT_FRAME_FINISHED = 0
EVENT_KEY_PRESSED = 1
EVENT_KEY_RELEASED = 2
EVENT_MOUSE_MOVED = 3
EVENT_MOUSE_PRESSED = 4
EVENT_MOUSE_RELEASED = 5

# Define the instruction types
MV_INSTRUCTION_CLEAR = 0
MV_INSTRUCTION_COORDS = 1
MV_INSTRUCTION_POWER_OFF = 2
MV_INSTRUCTION_POWER_ON = 3

# The key codes (scancodes)
PADDLE1_UP = 17
PADDLE1_DOWN = 31
PADDLE2_UP = 23
PADDLE2_DOWN = 37

# The game config
START_X_OFFSET = 128
PADDLE_WIDTH = 32
PADDLE_HEIGHT = 128 + 64
PADDLE_SPEED = 500
ARENA_WIDTH = 1920
ARENA_HEIGHT = 1080
BALL_RADIUS = 16
BALL_SPEED = 500


@dataclass
class Instruction:
    """ A instruction to be sent to minivector """
    type_byte: ctypes.c_uint8
    value: ctypes.c_uint32

    def __bytes__(self):
        return self.type_byte.to_bytes(1, byteorder="big") + self.value.to_bytes(4, byteorder="big")


@dataclass
class Event:
    """ An event to be received from minivector """
    type_byte: ctypes.c_uint8
    value: ctypes.c_uint64

    @classmethod
    def from_bytes(cls, data: bytes):
        type_byte = data[0]
        value = int.from_bytes(data[1:], byteorder="big")
        return cls(type_byte, value)

    def __bytes__(self):
        return self.type_byte.to_bytes(1, byteorder="big") + self.value.to_bytes(8, byteorder="big")


# The keys that are down
keys_down = []

# Create the named pipe
if not os.path.exists(EVENT_PIPE):
    os.mkfifo(EVENT_PIPE)
    print("Pipe created: " + EVENT_PIPE)

# Open the minivector instruction pipe
instruction_pipe = None

# The game state
game = {
    "running": True,
    "score": [0, 0],
    "paddle1": {
        "position": [START_X_OFFSET, ARENA_HEIGHT / 2 - PADDLE_HEIGHT / 2],
    },
    "paddle2": {
        "position": [ARENA_WIDTH - START_X_OFFSET - PADDLE_WIDTH, ARENA_HEIGHT / 2 - PADDLE_HEIGHT / 2],
    },
    "ball": {
        "position": [ARENA_WIDTH / 2 - BALL_RADIUS, ARENA_HEIGHT / 2 - BALL_RADIUS],
        "velocity": [BALL_SPEED, BALL_SPEED / 2],
    },
}


def process_events(event_pipe: BufferedReader):
    """ Process the events that came in this frame. Break when the frame is finished """
    frame_finished = False
    while not frame_finished:
        data = event_pipe.read(EVENT_SIZE)
        if not data:
            continue

        events = []
        for i in range(0, len(data), EVENT_SIZE):
            events.append(Event.from_bytes(data[i:i + EVENT_SIZE]))

        for event in events:
            event = Event.from_bytes(data)
            if event.type_byte == EVENT_KEY_PRESSED:
                if event.value not in keys_down:
                    keys_down.append(event.value)
            elif event.type_byte == EVENT_KEY_RELEASED:
                if event.value in keys_down:
                    keys_down.remove(event.value)
            if event.type_byte == EVENT_FRAME_FINISHED:
                frame_finished = True

def update_game(delta):
    """ Update the game state """

    # Update the ball position
    ball = game["ball"]
    ball["position"][0] += ball["velocity"][0] * delta
    ball["position"][1] += ball["velocity"][1] * delta

    # Check for collisions with the paddles
    for paddle in [game["paddle1"], game["paddle2"]]:
        if ball["position"][0] < paddle["position"][0] + PADDLE_WIDTH and ball["position"][0] + BALL_RADIUS > paddle["position"][0]:
            if ball["position"][1] < paddle["position"][1] + PADDLE_HEIGHT and ball["position"][1] + BALL_RADIUS > paddle["position"][1]:
                # Move the ball back
                if ball["velocity"][0] > 0:
                    ball["position"][0] = paddle["position"][0] - BALL_RADIUS
                else:
                    ball["position"][0] = paddle["position"][0] + PADDLE_WIDTH

                ball["velocity"][0] = -ball["velocity"][0]

    # Check for collisions with the top and bottom of the arena
    if ball["position"][1] < 0 or ball["position"][1] > ARENA_HEIGHT:
        ball["velocity"][1] = -ball["velocity"][1]

    # Check for collisions with the left and right of the arena
    if ball["position"][0] < 0 or ball["position"][0] > ARENA_WIDTH:
        if ball["position"][0] < 0:
            game["score"][1] += 1
        else:
            game["score"][0] += 1

        # Reset the ball position
        ball["position"] = [ARENA_WIDTH / 2 -
                            BALL_RADIUS, ARENA_HEIGHT / 2 - BALL_RADIUS]

    # Update the paddle positions
    if PADDLE1_UP in keys_down:
        game["paddle1"]["position"][1] += PADDLE_SPEED * delta
    if PADDLE1_DOWN in keys_down:
        game["paddle1"]["position"][1] -= PADDLE_SPEED * delta
    if PADDLE2_UP in keys_down:
        game["paddle2"]["position"][1] += PADDLE_SPEED * delta
    if PADDLE2_DOWN in keys_down:
        game["paddle2"]["position"][1] -= PADDLE_SPEED * delta

    # Clamp the paddles to the arena
    for paddle in [game["paddle1"], game["paddle2"]]:
        if paddle["position"][1] < 0:
            paddle["position"][1] = 0
        if paddle["position"][1] > ARENA_HEIGHT - PADDLE_HEIGHT:
            paddle["position"][1] = ARENA_HEIGHT - PADDLE_HEIGHT

    # If the escape key is pressed, reset the score and the ball position
    if 1 in keys_down:
        game["score"] = [0, 0]
        game["ball"]["position"] = [ARENA_WIDTH / 2 -
                                    BALL_RADIUS, ARENA_HEIGHT / 2 - BALL_RADIUS]


def render_game():
    """ Render the game state """
    def render_paddle(position):
        positions = [
            (position[0], position[1]),
            (position[0] + PADDLE_WIDTH, position[1]),
            (position[0] + PADDLE_WIDTH, position[1] + PADDLE_HEIGHT),
            (position[0], position[1] + PADDLE_HEIGHT),
            (position[0], position[1]),
            (position[0] + PADDLE_WIDTH, position[1]),
        ]
        instructions = []
        for position in positions:
            x = int(position[0])
            if x < 0:
                x = 0
            if x > ARENA_WIDTH:
                x = ARENA_WIDTH
            y = int(position[1])
            if y < 0:
                y = 0
            if y > ARENA_WIDTH:
                y = ARENA_WIDTH
            coords = (x << 16) | y
            instructions.append(Instruction(MV_INSTRUCTION_COORDS, coords))

        # Add a turn on instruction after the first instruction
        instructions.insert(1, Instruction(MV_INSTRUCTION_POWER_ON, 0))
        # Add a turn off instruction after the last instruction
        instructions.append(Instruction(MV_INSTRUCTION_POWER_OFF, 0))

        return instructions

    def render_ball(position):
        """ Render the ball as a circle """
        instructions = []
        for i in range(0, 360, 5):
            x = position[0] + BALL_RADIUS * math.cos(math.radians(i))
            y = position[1] + BALL_RADIUS * math.sin(math.radians(i))
            x = int(x)
            if x < 0:
                x = 0
            if x > ARENA_WIDTH:
                x = ARENA_WIDTH
            y = int(y)
            if y < 0:
                y = 0
            if y > ARENA_WIDTH:
                y = ARENA_WIDTH
            coords = (x << 16) | y
            instructions.append(Instruction(MV_INSTRUCTION_COORDS, coords))

        # Add a turn on instruction after the first instruction
        instructions.insert(1, Instruction(MV_INSTRUCTION_POWER_ON, 0))
        # Add a turn off instruction after the last instruction
        instructions.append(Instruction(MV_INSTRUCTION_POWER_OFF, 0))

        return instructions

    def render_arena():
        """ Renders a border and a center line """
        instructions = []

        # Render the center line
        for i in range(0, ARENA_HEIGHT, 10):
            instructions.append(Instruction(
                MV_INSTRUCTION_COORDS, int((ARENA_WIDTH / 2)) << 16 | i))

        # Insert a turn on instruction after the first instruction
        instructions.insert(1, Instruction(MV_INSTRUCTION_POWER_ON, 0))
        # Insert a turn off instruction after the last instruction
        instructions.append(Instruction(MV_INSTRUCTION_POWER_OFF, 0))
        return instructions

    def render_score(score, dir=1):
        """ Render the score as a set of lines """
        instructions = []
        x_offset = 0
        y_offset = 0
        line_length = 64
        line_offset = 10
        line_spacing = 20
        for i in range(0, score):
            # Draw a line
            x1 = int((ARENA_WIDTH / 2) + dir *
                     ((x_offset + 10) * line_spacing))
            if x1 < (x_offset * line_offset / 2) or x1 > (ARENA_WIDTH - x_offset * line_offset / 2):
                x_offset = 0
                y_offset += 1
                x1 = int((ARENA_WIDTH / 2) + dir *
                         ((x_offset + 10) * line_spacing))

            y1 = ARENA_HEIGHT - line_offset - \
                (y_offset * (line_length + line_offset))
            if y1 < 0:
                break

            x2 = x1
            y2 = y1 - line_length

            coords1 = (x1 << 16) | y1
            instructions.append(Instruction(MV_INSTRUCTION_COORDS, coords1))

            instructions.append(Instruction(MV_INSTRUCTION_POWER_ON, 0))

            coords2 = (x2 << 16) | y2
            instructions.append(Instruction(MV_INSTRUCTION_COORDS, coords2))
            instructions.append(Instruction(MV_INSTRUCTION_COORDS, coords1))

            instructions.append(Instruction(MV_INSTRUCTION_POWER_OFF, 0))

            x_offset += 1

        return instructions

    instructions = []
    instructions.extend(render_paddle(game["paddle1"]["position"]))
    instructions.extend(render_paddle(game["paddle2"]["position"]))
    instructions.extend(render_ball(game["ball"]["position"]))
    instructions.extend(render_arena())
    instructions.extend(render_score(game["score"][0], -1))
    instructions.extend(render_score(game["score"][1], 1))

    instruction_bytes = b"".join(bytes(instruction)
                                 for instruction in instructions)
    return instruction_bytes


def main():
    global instruction_pipe

    print("Waiting for minivector to start...")

    event_pipe = open(EVENT_PIPE, "rb")
    instruction_pipe = open(INSTRUCTION_PIPE, "wb")
    os.set_blocking(event_pipe.fileno(), False)
    os.set_blocking(instruction_pipe.fileno(), False)

    print("Minivector started")

    start = time.time()
    delta = 0
    while game["running"]:
        # Process the events and wait for the frame to finish
        update_game(delta)
        instructions = render_game()
        process_events(event_pipe)
        instruction_pipe.write(instructions)
        instruction_pipe.flush()
        delta = time.time() - start
        start = time.time()

    print("Minivector stopped")


if __name__ == "__main__":
    main()
