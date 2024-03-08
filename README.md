# MiniVector
A configurable XY vector display emulator

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
