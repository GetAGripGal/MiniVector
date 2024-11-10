# MiniVector

A programmable XY vector display simulator.
Written in rust and wgpu.

![minivector_demo](docs/radar.gif)

## Examples

For examples of minivector in action, there are some tests:
* [Bad Apple](docs/badapple.md): Display bad apple in minivector
* [Pong](docs/pong.md): Play pong in minivector

## OS support

Right now only unix systems are supported. Windows support is planned in the future.
Wasm32 support was inteded, but is currently not possible since some native-only wgpu features are utilized.

## Performance

Minivector is very gpu heavy. 
If you are processing a lot of points, a mid-range gpu is reccomended.

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
        -w,  --window <width> <height>          Set the window size
            default: 1280 720
        -f   --fullscreen                       Set the window to fullscreen [Can be toggled with F11]
    display
        -r,  --resolution <width> <height>      Set the resolution
            default: 1920 1080
        -p,  --primary <color_hex>              Set the primary (background) color 
            default: 282828
        -s,  --secondary <color_hex>            Set the secondary (background) color
            default: 33ff64
        -ss, --screen-size <width> <height>     Set the screen size 
            default: [same as the resolution]
    gun
        -rg, --radius <radius>                  Set the radius of the electron gun
            default: 1.0
        -df, --dim-factor <factor>              Set the dim factor per frame
            default: 0.3
    executor:
        -ip,  --instruction-pipe <pipe>         Set the pipe to read the instructions 
            default (unix): /tmp/mv_pipe
            default (windows): \\\\.\\pipe\\mv_pipe
        -ep,  --event-pipe <pipe>               Set the pipe to send the events
            defaylt: none
        -ifr,  --instruction-per-frame <n>      Set the number of instructions per frame [If 0, it will execute all instructions in the buffer in one frame]
            default: 500
        -fr, --frame-rate <n>                   Set the frame rate
            default: vsync
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
- 0b00 : Clear display      [No Data]
- 0b01 : Move to position   [X: ii6, Y: i16]
- 0b10 : Electron gun on    [No Data]
- 0b11 : Electron gun off   [No Data]
```

## Events

For interactions with another program/script, the event pipe can be used.
Events are 74 bits of data.

- The first 8-bits indicate the instruction type.
- The the rest is a 64 bit buffer of addtional data

```
| 00000000 0000000000000000000000000000000000000000000000000000000000000000
| -------- ----------------------------------------------------------------
     |- 8-bit Instruction   |- 64-bit Additional Data
```

For an example of how to use event types, take a look at the [pong demo](docs/pong.md).

### Event Types

```
- 0b000 : Frame finished processing  [No Data]
- 0b001 : Key pressed                [Key Scancode]
- 0b010 : Key released               [Key Scancode]
- 0b011 : Mouse moved to position    [X: u32, Y: u32]
- 0b100 : Mouse button pressed       [Mouse button]
- 0b101 : Mouse button released      [Mouse button]
```