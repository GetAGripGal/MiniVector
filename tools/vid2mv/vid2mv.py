"""
Converts a video file to a set of instructions to be rendered by minivector
"""

import ctypes
from dataclasses import dataclass
import sys
import cv2
import os
import pickle
import tempfile

USAGE_MESSAGE = """Usage: python vid2mv.py <video_file> <output_file> [options]
    -p, --points_per_frame <int>            Number of points to generate per frame
    -i, --instructions_per_frame <int>      Number of instructions to generate per frame
    -u, --upscaling_factor <int>            Upscaling factor for the output
    -d, --downscaling_factor <int>          Downscaling factor for the input
"""

args = {
    "video_file": None,
    "output_file": None,
    "points_per_frame": 150,
    "instructions_per_frame": 200,
    "upscaling_factor": 1,
    "downscaling_factor": 1
}

# Create a temporary directory to store the frames
temp_dir = tempfile.TemporaryDirectory()
temp_dir_path = temp_dir.name

# The resolution of the input video
resolution = []


@dataclass
class Instruction:
    """ A instruction to be sent to minivector """
    type_byte: ctypes.c_uint8
    value: ctypes.c_uint32

    def __bytes__(self):
        return self.type_byte.to_bytes(1, byteorder="big") + self.value.to_bytes(4, byteorder="big")


def parse_args():
    """ Parse command line arguments """
    i = 1
    while i < len(sys.argv):
        arg = sys.argv[i]
        if arg in ["-p", "--points_per_frame"]:
            args["points_per_frame"] = int(sys.argv[i + 1])
            i += 1
        elif arg in ["-i", "--instructions_per_frame"]:
            args["instructions_per_frame"] = int(sys.argv[i + 1])
            i += 1
        elif arg in ["-u", "--upscaling_factor"]:
            args["upscaling_factor"] = int(sys.argv[i + 1])
            i += 1
        elif arg in ["-d", "--downscaling_factor"]:
            args["downscaling_factor"] = int(sys.argv[i + 1])
            i += 1
        else:
            if i == 1:
                args["video_file"] = arg
            elif i == 2:
                args["output_file"] = arg
            else:
                print("Invalid argument: " + arg)
                print(USAGE_MESSAGE)
                sys.exit(1)
        i += 1

    if "video_file" not in args or "output_file" not in args:
        print(USAGE_MESSAGE)
        sys.exit(1)


def video_to_frames():
    """ Extract frames from the video """
    global resolution
    vidcap = cv2.VideoCapture(args["video_file"])
    success, image = vidcap.read()
    count = 0
    while success:
        if len(resolution) == 0:
            resolution = [image.shape[1], image.shape[0]]
        cv2.imwrite(f"{temp_dir_path}/frame_{count}.jpg", image)
        success, image = vidcap.read()
        count += 1
    return count


def downscale_image(image, factor):
    """
    Downscale an image by a factor
    """
    return cv2.resize(image, dsize=(0, 0), fx=1/factor, fy=1/factor)


def downscale_images_in_temp_dir(factor):
    """
    Downscale all the images in the temporary directory
    """
    for i in os.listdir(temp_dir_path):
        image = cv2.imread(f"{temp_dir_path}/{i}")
        image = downscale_image(image, factor)
        cv2.imwrite(f"{temp_dir_path}/{i}", image)


def threshold_image(image):
    """
    Convert an image to a binary image
    """
    gray = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)
    _, thresh = cv2.threshold(
        gray, 127, 255, cv2.THRESH_BINARY + cv2.THRESH_OTSU)
    return thresh


def find_contours(image):
    """
    Find the contours in a binary image
    """
    contours, _ = cv2.findContours(
        image, cv2.RETR_TREE, cv2.CHAIN_APPROX_SIMPLE)
    return contours


def extract_contours(image):
    """
    Extracts contours from an image
    """
    thresh = threshold_image(image)
    contours = find_contours(thresh)
    return contours


def extract_countours_from_temp_dir():
    """
    Extracts contours from all the frames in the temporary directory
    """
    frames = []
    for frame in sorted(os.listdir(temp_dir_path), key=lambda x: int(x.split("_")[1].split(".")[0])):
        print(f"Extracting contours from {frame}")
        image = cv2.imread(os.path.join(temp_dir_path, frame))
        contours = extract_contours(image)
        frames.append(contours)
    return frames


def flatten_frames(frames):
    """
    Flatten a set of frames into a single list of points
    """
    final_frames = []
    for frame in frames:
        final_frame = []
        for contour in frame:
            points = []
            for point in contour:
                x, y = point[0]
                points.append((x, resolution[1] - y))
            final_frame.append(points)
        final_frames.append(final_frame)
    return final_frames


def limit_frames(frames, limit):
    """
    Limit the number of points in a set of frames
    """
    final_frames = []
    for frame in frames:
        final_frame = []
        for contour in frame:
            if len(contour) > limit:
                step = len(contour) // limit
                final_frame.append(contour[::step])
            else:
                final_frame.append(contour)
        final_frames.append(final_frame)
    return final_frames


def upscale_image(contours):
    """ 
    Upscale the coordinates, we also need to move them back to the top left corner.
    """
    if len(contours) == 0:
        return []

    upscale_factor = args["upscaling_factor"]
    print(f"Upscaling by {upscale_factor}")
    new_contours = []

    for contour in contours:
        new_contour = []
        for point in contour:
            x, y = point
            new_x = x * upscale_factor
            new_y = y * upscale_factor
            print(
                f"Point: {x}, {y}, Scale: {upscale_factor}, New: {new_x}, {new_y}")
            new_contour.append((new_x, new_y))
        new_contours.append(new_contour)

    return new_contours


def convert_to_instructions(contours):
    """ Convert a frame of contours to a set of instructions """
    instructions = []
    instructions_per_frame = args["instructions_per_frame"]
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
    first = True
    while len(instructions) < instructions_per_frame:
        if first:
            instructions.append(Instruction(2, 0))
            first = False
        else:
            instructions.append(Instruction(1, 0))

    return instructions


def main():
    parse_args()
    frame_count = video_to_frames()
    print(f"Extracted {frame_count} frames from the video")

    downscale_images_in_temp_dir(args["downscaling_factor"])
    print("Downscaled all the frames")

    frames = extract_countours_from_temp_dir()
    print("Extracted contours from all the frames")

    frames = flatten_frames(frames)
    print("Flattened all the frames")

    frames = limit_frames(frames, args["points_per_frame"])
    print("Limited the number of points in all the frames")

    instructions = []
    for frame in frames:
        frame = upscale_image(frame)
        instructions.extend(convert_to_instructions(frame))
    print("Converted all the frames to instructions")

    with open(args["output_file"], "wb") as file:
        for instruction in instructions:
            file.write(bytes(instruction))
    temp_dir.cleanup()

    upscale_factor = args["upscaling_factor"]
    print(f"Upscaling by {upscale_factor}")


if __name__ == "__main__":
    main()
