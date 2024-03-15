import ctypes
from dataclasses import dataclass
import tkinter as tk
import threading
import time


MV_INSTRUCTION_CLEAR = 0
MV_INSTRUCTION_COORDS = 1
MV_INSTRUCTION_POWER_OFF = 2
MV_INSTRUCTION_POWER_ON = 3

"""
The font is stored as units. These units describe the font in a 2D grid. And whether the electron gun is powered at that point.
"""
FONT = {
    "A": [
        [0, 0, False],
        [0, 2, True],
        [1, 2, True],
        [1, 0, True],
        [1, 1, True],
        [0, 1, True],
        [1, 1, False],
    ],
    "B": [
        [0, 0, False],
        [0, 2, True],
        [1, 2, True],
        [1, 1, True],
        [0, 1, True],
        [1, 1, True],
        [1, 0, True],
        [0, 0, True],
        [0, 0, False],
    ],
    "C": [
        [1, 0, False],
        [0, 0, True],
        [0, 2, True],
        [1, 2, True],
        [1, 0, False],
    ],
    "D": [
        [0, 0, False],
        [0, 2, True],
        [1, 2, True],
        [1, 0, True],
        [0, 0, True],
        [0, 0, False],
    ],
    "E": [
        [1, 0, False],
        [0, 0, True],
        [0, 2, True],
        [1, 2, True],
        [1, 1, False],
        [0, 1, True],
        [1, 1, False],
    ],
    "F": [
        [0, 0, False],
        [0, 2, True],
        [1, 2, True],
        [1, 1, False],
        [0, 1, True],
    ],
    "G": [
        [0, 0, False],
        [1, 0, True],
        [1, 2, True],
        [0, 2, True],
        [0, 1, True],
        [1, 1, True],
        [1, 1, False],
    ],
    "H": [
        [0, 0, False],
        [0, 2, True],
        [0, 1, True],
        [1, 1, True],
        [1, 0, True],
        [1, 2, True],
        [1, 1, False],
    ],
    "I": [
        [0, 0, False],
        [1, 0, True],
        [0, 0, True],
        [0, 2, True],
        [1, 2, True],
        [0, 2, False],
    ],
    "J": [
        [1, 0, False],
        [1, 2, True],
        [0, 2, True],
        [0, 0, True],
        [1, 0, True],
    ],
    "K": [
        [0, 0, False],
        [0, 2, True],
        [0, 1, True],
        [1, 0, True],
        [0, 1, True],
        [1, 2, True],
    ],
    "L": [
        [0, 0, False],
        [0, 2, True],
        [1, 2, True],
        [0, 2, False],
    ],
    "M": [
        [0, 0, False],
        [0, 2, True],
        [1, 1, True],
        [1, 2, True],
        [1, 0, True],
        [1, 2, False],
    ],
    "N": [
        [0, 0, False],
        [0, 2, True],
        [1, 0, True],
        [1, 2, True],
        [1, 0, False],
    ],
    "O": [
        [0, 0, False],
        [0, 2, True],
        [1, 2, True],
        [1, 0, True],
        [0, 0, True],
        [0, 0, False],
    ],
    "P": [
        [0, 0, False],
        [0, 2, True],
        [1, 2, True],
        [1, 1, True],
        [0, 1, True],
        [0, 0, True],
    ],
    "Q": [
        [0, 0, False],
        [0, 2, True],
        [1, 2, True],
        [1, 0, True],
        [0, 0, True],
        [0, 0, False],
        [1, 1, True],
    ],
    "R": [
        [0, 0, False],
        [0, 2, True],
        [1, 2, True],
        [1, 1, True],
        [0, 1, True],
        [0, 0, True],
        [1, 2, False],
    ],
    "S": [
        [1, 0, False],
        [0, 0, True],
        [0, 1, True],
        [1, 1, True],
        [1, 2, True],
        [0, 2, False],
    ],
    "T": [
        [0, 0, False],
        [1, 0, True],
        [0, 0, True],
        [0, 2, True],
        [0, 1, False],
    ],
    "U": [
        [0, 0, False],
        [0, 2, True],
        [1, 2, True],
        [1, 0, True],
        [0, 0, True],
        [1, 2, False],
    ],
    "V": [
        [0, 0, False],
        [0, 2, True],
        [1, 2, True],
        [1, 0, True],
        [1, 2, False],
    ],
    "W": [
        [0, 0, False],
        [0, 2, True],
        [1, 2, True],
        [1, 0, True],
        [1, 2, True],
        [1, 0, False],
    ],
    "X": [
        [0, 0, False],
        [1, 2, True],
        [1, 0, True],
        [0, 2, True],
        [1, 0, False],
    ],
    "Y": [
        [0, 0, False],
        [0, 1, True],
        [1, 1, True],
        [0, 1, True],
        [0, 2, True],
        [0, 1, False],
    ],
    "Z": [
        [0, 0, False],
        [1, 0, True],
        [0, 2, True],
        [1, 2, True],
        [0, 2, False],
    ],
}

text_lock = threading.Lock()
text = ""

conf_lock = threading.Lock()
conf = {
    "size": 20,
    "position": [20, 1000],
}

running_lock = threading.Lock()
running = True


@dataclass
class Instruction:
    """ A instruction to be sent to minivector """
    type_byte: ctypes.c_uint8
    value: ctypes.c_uint32

    def __bytes__(self):
        return self.type_byte.to_bytes(1, byteorder="big") + self.value.to_bytes(4, byteorder="big")


class FontRenderer:
    """ A font renderer that can render text to a list of instructions"""

    def __init__(self, font):
        self.font = font
        self.on = False
        self.line = 0

    def render(self, text, position, size):
        """Render the text to a list of instructions"""
        instructions = []
        for i, char in enumerate(text):
            if char in self.font:
                for x, y, on in self.font[char]:
                    if on != self.on:
                        if on:
                            instruction = Instruction(
                                MV_INSTRUCTION_POWER_ON, 0)
                        else:
                            instruction = Instruction(
                                MV_INSTRUCTION_POWER_OFF, 0)
                        instructions.append(instruction)
                        self.on = on

                    # Scaled, translated and positioned based on the size and position and the index of the character
                    x = int(x * size + position[0] + i * size * 1.5)
                    y = int(y * size + position[1] - self.line * size * 1.5)
                    coords = (x << 16) | y
                    instruction = Instruction(MV_INSTRUCTION_COORDS, coords)
                    instructions.append(instruction)
            if char == "\n":
                self.line += 1
                self.on = False
        self.line = 0

        return instructions


def quit():
    global running
    with running_lock:
        running = False


def send_instructions(instructions):
    with open("/tmp/mv_pipe", "wb") as file:
        for instruction in instructions:
            file.write(bytes(instruction))
        file.write(bytes(Instruction(MV_INSTRUCTION_POWER_OFF, 0)))


def update_text(text_editor, size_editor, position_editor):
    global text
    with text_lock:
        text = text_editor.get("1.0", "end-1c")
    with conf_lock:
        conf["size"] = int(size_editor.get())
        conf["position"] = [int(x) for x in position_editor.get().split(",")]


def window_loop():
    def quit():
        """Quit the application."""
        global running
        with running_lock:
            running = False
            window.destroy()

    window = tk.Tk()

    window.title("Font Rendering Test")
    window.geometry("800x600")

    text_editor = tk.Text(window, font=("Arial", 12))

    size_label = tk.Label(window, text="Size")
    position_label = tk.Label(window, text="Position")
    size_editor = tk.Entry(window)
    position_editor = tk.Entry(window)

    size_editor.insert(0, "20")
    position_editor.insert(0, "20, 100")

    button = tk.Button(window, text="Render",
                       command=lambda: update_text(text_editor, size_editor, position_editor))

    size_label.pack()
    size_editor.pack()
    position_label.pack()
    position_editor.pack()
    text_editor.pack(expand=True, fill="both")

    button.pack()

    window.protocol("WM_DELETE_WINDOW", lambda: quit())
    window.mainloop()


def main():
    global running

    window_thread = threading.Thread(target=window_loop)
    window_thread.start()

    should_run = True

    font_renderer = FontRenderer(FONT)

    sleep = 1 / 60
    while should_run:
        instructions = font_renderer.render(
            text, conf["position"], conf["size"])
        send_instructions(instructions)

        with text_lock:
            print(text)
        with running_lock:
            should_run = running
        time.sleep(sleep)


if __name__ == "__main__":
    main()
