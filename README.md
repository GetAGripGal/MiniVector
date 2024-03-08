# MiniVector

A configurable XY vector display simulator

![minivector_demo](docs/minivector_demo.png)

## TODO

- Open port for streaming point data
- Add shader to simulate a vector display

## Usage

```
Usage: microvector [options]
  Options:
  -w, --window <width> <height>     Set the window size
  -r, --resolution <width> <height> Set the resolution
  -p, --primary <color_hex>         Set the primary color
  -s, --secondary <color_hex>       Set the secondary color
  -l, --line-width <width>          Set the line width
  -h, --help                        Show this help message
```

## Instructions

The simulator responds to a specific format of instructions.
Instructions are 38 bits of data.

- The first 8-bits indicate the instruction type.
- The the rest is a 32 bit buffer of addtional data (eg. point coorinates)

```
00000000 00000000000000000000000000000000
-------- --------------------------------
    |- Instruction      |- Additional Data
```

### Instruction Set

The following is a list of supported instructions and their additional data

```
- 0x00000000 : Clear display    [No Data]
- 0x00000001 : Add point        [X: u32, Y: u32]
```

## Building

This repo uses premake. The additional libraries are stored in vendor.

### Requirements

MiniVector must be linked to `glfw3`.

### Cloning

You can clone the repo and all its repositories with `--recursive`:

```bash
git clone https://github.com/ComLarsic/MiniVector.git --recursive
```

### Build

Generate the makefiles using premake:

```bash
premake5 gmake
```

Then build it using make

```bash
cd build
make
```
