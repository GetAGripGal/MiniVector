"""
A simple script that writes a file to the pipe.
This script can be usedd to introduce a delay between the instructions.
"""

from dataclasses import dataclass
import ctypes
import time
import sys
import math

MV_INSTRUCTION_CLEAR = 0
MV_INSTRUCTION_COORDS = 1
MV_INSTRUCTION_POWER_OFF = 2
MV_INSTRUCTION_POWER_ON = 3

USAGE = """
Usage: python3 write.py <file> <delay_ms>
"""

instruction_dump = []


@dataclass
class Instruction:
    type_byte: ctypes.c_uint8
    value: ctypes.c_uint32

    def __bytes__(self):
        return self.type_byte.to_bytes(1, byteorder="big") + self.value.to_bytes(4, byteorder="big")


def clear_screen():
    instruction = Instruction(MV_INSTRUCTION_CLEAR, 0)
    instruction_dump.append(instruction)


def write_coords(x, y):
    coords = (x << 16) | y
    instruction = Instruction(MV_INSTRUCTION_COORDS, coords)
    instruction_dump.append(instruction)


def power_off():
    instruction = Instruction(MV_INSTRUCTION_POWER_OFF, 0)
    instruction_dump.append(instruction)


def power_on():
    instruction = Instruction(MV_INSTRUCTION_POWER_ON, 0)
    instruction_dump.append(instruction)


def read_conf():
    if len(sys.argv) != 3:
        print(USAGE)
        sys.exit(1)
    return sys.argv[1], int(sys.argv[2])


def smiley():
    write_coords(100 * 2, 32 * 5)

    power_on()
    write_coords(100 * 2, 64 * 5)
    write_coords(100 * 2, 32 * 5)

    power_off()
    write_coords(132 * 2, 64 * 5)

    power_on()
    write_coords(132 * 2, 32 * 5)

    power_off()
    write_coords(132 * 2, 16 * 5)

    power_on()
    write_coords(100 * 2, 16 * 5)
    write_coords(92 * 2, 24 * 5)
    power_off()
    write_coords(140 * 2, 24 * 5)
    power_on()
    write_coords(132 * 2, 16 * 5)

    time.sleep(.5)


def circle(cx, cy, r, num_points=100):
    """ Draw a circle by adding many points """
    first = True
    for i in range(num_points):
        theta = 2 * math.pi * i / num_points
        x = cx + r * math.cos(theta)
        y = cy + r * math.sin(theta)
        if first:
            power_off()
            write_coords(int(x), int(y))
            first = False
            continue
        power_on()
        write_coords(int(x), int(y))


def wave(y, scan_rate, amp):
    """ Draw a wave """
    for x in range(0, 1920, 2):
        power_on()
        write_coords(x, int(y + amp * math.sin(x / scan_rate)))


def radar_line(x, y, length, angle):
    """ Draw a line """
    power_off()
    write_coords(int(x), int(y))
    power_on()
    write_coords(int(x + length * math.cos(angle)),
                 int(y + length * math.sin(angle)))


def hello_world():
    """ Draw a hello world """

    # H
    power_off()
    write_coords(100, 800)
    power_on()
    write_coords(100, 900)
    power_off()
    write_coords(100, 850)
    power_on()
    write_coords(150, 850)
    power_off()
    write_coords(150, 800)
    power_on()
    write_coords(150, 900)

    # E
    power_off()
    write_coords(200, 800)
    power_on()
    write_coords(200, 875)
    write_coords(250, 875)
    power_off()
    write_coords(200, 850)
    power_on()
    write_coords(250, 850)
    power_off()
    write_coords(200, 800)
    power_on()
    write_coords(250, 800)

    # L
    power_off()
    write_coords(300, 875)
    power_on()
    write_coords(300, 800)
    write_coords(350, 800)

    # L
    power_off()
    write_coords(400, 875)
    power_on()
    write_coords(400, 800)
    write_coords(450, 800)

    # O
    power_off()
    write_coords(500, 800)
    power_on()
    write_coords(500, 875)
    write_coords(550, 875)
    write_coords(550, 800)
    write_coords(500, 800)
    write_coords(550, 800)

    # W
    power_off()
    write_coords(650, 800)
    power_on()
    write_coords(650, 900)
    power_off()
    write_coords(650, 800)
    power_on()
    write_coords(675, 800)
    write_coords(675, 900)
    power_off()
    write_coords(675, 800)
    power_on()
    write_coords(700, 800)
    write_coords(700, 900)

    # O
    power_off()
    write_coords(750, 800)
    power_on()
    write_coords(750, 875)
    write_coords(800, 875)
    write_coords(800, 800)
    write_coords(750, 800)
    write_coords(800, 800)

    # R
    power_off()
    write_coords(850, 800)
    power_on()
    write_coords(850, 875)
    write_coords(900, 875)

    # L
    power_off()
    write_coords(950, 875)
    power_on()
    write_coords(950, 800)
    write_coords(1000, 800)

    # D
    power_off()
    write_coords(1025, 800)
    power_on()
    write_coords(1025, 850)
    write_coords(1075, 850)
    write_coords(1075, 900)
    write_coords(1075, 800)
    write_coords(1025, 800)


def write_instructions():
    print("Writing {i} instructions", len(instruction_dump))
    with open("dump.mv", "wb") as f:
        for instruction in instruction_dump:
            f.write(bytes(instruction))


def clear_instructions():
    with open("/tmp/mv_pipe", "wb") as f:
        f.truncate(0)


def main():
    # for i in range(360):
    #     wave(1080 / 2, 50, 50 + math.sin(i) * 100)
    #     write_coords(0, int(600 / 2))
    #     circle(1920 / 2, 1080 / 2, 100, 200)
    #     circle(1920 / 2, 1080 / 2, 200, 200)
    #     circle(1920 / 2, 1080 / 2, 300, 200)
    #     circle(1920 / 2, 1080 / 2, 400, 200)
    #     circle(1920 / 2, 1080 / 2, 500, 200)
    #     radar_line(1920 / 2, 1080 / 2, 500, math.radians(i))
    #     power_off()
    #     smiley()
    #     power_off()
    #
    #     print(f"pass: {i}")
    hello_world()

    power_off()
    for i in range(500):
        write_coords(0, 0)
        write_coords(0, 1)

    write_instructions()


if __name__ == "__main__":
    main()
