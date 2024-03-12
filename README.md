# MiniVector

A configurable XY vector display simulator for unix-like systems.

![minivector_demo](docs/radar.gif)

## TODO

- Improve accuracy of fade-out
- Add pipe for input and event polling (This will allow other applications to interact with the window / keyboard input)
- Improve instruction reading by making it more performant
- Windows support

### Pie in the sky

There are some non-essential features I would like to add in the future.

- [Windows 96](https://windows96.net/) support
- Seperate utility for font rendering
- Seperate utility for vector art drawing

## Usage

Minivector will create a pipe at the specified location.
Applications can write instructions to this pipe which will be executed by minivector.

Example

```bash
nohup minivector &                                 # Run detached
cat test/drawings/hello_world.mv >> /tmp/mv_pipe   # Send instructions
```

### Options

```
usage: ./bin/Debug/minivector [options]
options:
    window:
      -w,  --window <width> <height>     Set the window size
      -f   --fullscreen                  Set the window to fullscreen
    display:
      -r,  --resolution <width> <height> Set the resolution
      -p,  --primary <color_hex>         Set the primary color
      -s,  --secondary <color_hex>       Set the secondary color
    executor:
      -i,  --pipe <pipe>                 Set the pipe to read the instructions
      -e,  --instruction-per-frame <n>   Set the number of instructions per frame
    legacy:
      -le, --legacy                      Use the legacy renderer
      -l,  --line-width <width>          Set the line width
      -h,  --help                        Show this help message
```

## Instructions

The simulator responds to a specific format of instructions.
Instructions are 40 bits of data.

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
- 0x00000000 : Clear display      [No Data]
- 0x00000001 : Move to position   [X: ii6, Y: i16]
- 0x00000010 : Electron gun on    [No Data]
- 0x00000011 : Electron gun off   [No Data]
```

### Legacy Mode

Legacy mode is the original GL_LINES-based proof of concept.
This mode is perserved for because I like it :3

This mode supports a limited subset of instructions

```
- 0x00000000 : Clear display      [No Data]
- 0x00000001 : Move to position   [X: ii6, Y: i16]
```

## Building

This repo uses premake. The additional libraries are stored in vendor.

### Cloning

You can clone the repo and all its repositories with `--recursive`:

```bash
git clone https://github.com/ComLarsic/MiniVector.git --recursive
```

### Build

Generate the makefiles using premake.
Then build it using make:

```bash
premake5 gmake
cd build
make
```
