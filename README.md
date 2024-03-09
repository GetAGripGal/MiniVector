# MiniVector

A programmable XY vector display simulator for unix-like systems.
Written using GLFW, OpenGl and [HandmadeMath](https://github.com/HandmadeMath/HandmadeMath).

![minivector_demo](docs/minivector_demo.png)

## TODO

Currently the simulator is nothing more than a line renderer.
However, later on I would like to implement full electron gun simulation.

There are several things required before full 1.0 release:

- Replace line rendering with compute shader raserizer
- Implement electron gun simulation instead of GL_LINES
- Change instruction set to work with electron gun movement instead of drawing points directly
- Add shader to simulate a vector display

### Pie in the sky

There are some non-essential features I would like to add in the future.

- [Windows 96](https://windows96.net/) support
- Windows support
- Seperate utility for font rendering
- Seperate utility for vector art drawing

## Usage

Minivector will create a pipe at the specified location.
Applications can write instructions to this pipe which will be executed by minivector.

Example

```bash
nohup minivector &                            # Run detached
cat test/drawings/smiley.mv >> /tmp/mv_pipe   # Send instructions
```

### Options

```s
Usage: microvector [options]
  Options:
  -w, --window <width> <height>     Set the window size
  -r, --resolution <width> <height> Set the resolution
  -p, --primary <color_hex>         Set the primary color
  -s, --secondary <color_hex>       Set the secondary color
  -l, --line-width <width>          Set the line width
  -i, --pipe <pipe>                 Set the pipe to read the instructions
  -h, --help                        Show this help message
```

## Instructions

The simulator responds to a specific format of instructions.
Instructions are 38 bits of data.

- The first 8-bits indicate the instruction type.
- The the rest is a 32 bit buffer of addtional data (eg. point coorinates)

```
| 00000000 00000000000000000000000000000000
| -------- --------------------------------
     |- 8-bit Instruction   |- 32-bit Additional Data
```

### Instruction Set

The following is a list of supported instructions and their additional data

```
- 0x00000000 : Clear display    [No Data]
- 0x00000001 : Add point        [X: ii6, Y: i16]
```

## Building

This repo uses premake. The additional libraries are stored in vendor.

### Cloning

You can clone the repo and all its repositories with `--recursive`:

```bash
git clone https://github.com/ComLarsic/MiniVector.git --recursive
```

### Building

Generate the makefiles using premake.
Then build it using make:

```bash
premake5 gmake
cd build
make
```
