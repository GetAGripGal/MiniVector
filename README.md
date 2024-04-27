# MiniVector

A programmable XY vector display simulator.
Written in rust and wgpu.

![minivector_demo](docs/legacy/radar.gif)

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
usage: minivector [options]
    options:
        window:
            -w,  --window <width> <height>          Set the window siz
            -f   --fullscreen                       Set the window to fullscree
        display
            -r,  --resolution <width> <height>      Set the resolutio
            -p,  --primary <color_hex>              Set the primary color
            -s,  --secondary <color_hex>            Set the secondary color
            -ss, --screen-size <width> <height>     Set the screen size [By default it is the same as the resolution]
        gun
            -rg, --radius <radius>                  Set the radius of the electron gun
            -df, --dim-factor <factor>              Set the dim factor per frame
        executor:
            -ip,  --instruction-pipe <pipe>         Set the pipe to read the instructions
                default (unix): /tmp/mv_pipe
                default (windows): \\\\.\\pipe\\mv_pipe
            -ep,  --event-pipe <pipe>               Set the pipe to send the events (none by default)
            -e,  --instruction-per-frame <n>        Set the number of instructions per frame [If not set, it will execute all instructions in the buffer at once]
            -fr, --frame-rate <n>                   Set the frame rate
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
