import ctypes
from dataclasses import dataclass
import cv2
import os
import sys

import numpy as np

args = {
    "image": "",
    "output": "",
}

resolution = [0, 0]


@dataclass
class Instruction:
    """ A instruction to be sent to minivector """
    type_byte: ctypes.c_uint8
    value: ctypes.c_uint32

    def __bytes__(self):
        return self.type_byte.to_bytes(1, byteorder="big") + self.value.to_bytes(4, byteorder="big")


def threshold_image(image):
    """
    Convert an image to a binary image
    """
    gray = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)

    # Reduce glow by blurring the image
    gray = cv2.GaussianBlur(gray, (5, 5), 0)
    _, thresh = cv2.threshold(
        gray, 0, 255, cv2.THRESH_BINARY_INV + cv2.THRESH_OTSU)
    cv2.imshow("Threshold", thresh)
    cv2.waitKey(0)
    return thresh


def find_contours(image):
    """
    Find the contours in a binary image
    """
    contours, _ = cv2.findContours(
        image, cv2.RETR_TREE, cv2.CHAIN_APPROX_SIMPLE)
    return contours


def flatten_frames(contours):
    """
    Flatten a list of frames into a list of contours
    """
    output = []
    for contour in contours:
        points = []
        for point in contour:
            x, y = point[0]
            points.append((x, resolution[1] - y))
        output.append(points)
    return output


def extract_contours(image):
    """
    Extracts contours from an image
    """
    thresh = threshold_image(image)
    contours = find_contours(thresh)
    return contours


def convert_to_instructions(contours):
    """ Convert a frame of contours to a set of instructions """
    instructions = []
    for contour in contours:
        # Turn off the pen bewteen each contour
        instructions.append(Instruction(2, 0))
        turned_off = True
        for point in contour:
            x, y = point
            coords = (int(x) << 16) | int(y)
            instructions.append(Instruction(1, int(coords)))
            if turned_off:
                instructions.append(Instruction(3, 0))
                turned_off = False

    return instructions


def read_args():
    """
    Read the arguments from the command line
    """
    if len(sys.argv) % 2 == 0:
        print("Usage: python img2mv.py <image> <output>")
        sys.exit(1)

    args["image"] = sys.argv[1]
    args["output"] = sys.argv[2]


def main():
    image = cv2.imread(args["image"])
    resolution[0] = image.shape[1]
    resolution[1] = image.shape[0]
    print(f"Resolution: {resolution[0]}x{resolution[1]}")
    contours = extract_contours(image)
    print(f"Extracted {len(contours)} contours")
    flattened = flatten_frames(contours)
    print(f"Flattened {len(flattened)} contours")
    instructions = convert_to_instructions(flattened)
    print(f"Writing {len(instructions)} instructions to {args['output']}")
    with open(args["output"], "wb") as f:
        for instruction in instructions:
            f.write(bytes(instruction))
    print("Done")


if __name__ == "__main__":
    read_args()
    main()
