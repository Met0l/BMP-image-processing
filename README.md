# BMP Image Processor

A command-line tool written in C that reads and edits 24-bit uncompressed BMP images, working directly on the raw file bytes with no external libraries.

## Features

| Command      | Description                                                    |
|--------------|----------------------------------------------------------------|
| `info`       | Print width, height, file size and bits per pixel              |
| `grayscale`  | Convert the image to grayscale (weighted luminance formula)    |
| `invert`     | Invert all colours (negative)                                  |
| `brightness` | Increase or decrease brightness by a percentage                |
| `blur`       | Apply a 3x3 box blur                                           |
| `crop`       | Cut out a rectangular region                                   |
| `rotate`     | Rotate the image by 90 or 180 degrees                          |

## Build

Requires a C compiler such as `gcc`:

```bash
gcc -o bmp_tool Grayscale_bmp_image_v2_functii.c
```

## Usage

```bash
./bmp_tool info       input.bmp
./bmp_tool grayscale  input.bmp output.bmp
./bmp_tool invert     input.bmp output.bmp
./bmp_tool brightness input.bmp output.bmp -50%
./bmp_tool blur       input.bmp output.bmp
./bmp_tool crop       input.bmp output.bmp 20 20 300 250
./bmp_tool rotate     input.bmp output.bmp 180
```

### Arguments

- **brightness**: a sign (`+` or `-`) followed by a percentage, e.g. `+30%` or `-50%`. Values are clamped to the 0-255 range.
- **crop**: `x y width height`, where `x` and `y` are the starting coordinates and `width`/`height` the size of the new image, in pixels. If the region goes outside the image, the program prints an error and exits.
- **rotate**: `90` or `180`.

## How it works

### BMP structure

A BMP file starts with two headers, followed by the pixel data:

1. `BITMAPFILEHEADER` (14 bytes): signature, file size, offset to the pixel data
2. `BITMAPINFOHEADER` (40 bytes): width, height, bits per pixel, compression, etc.
3. Pixel data, stored as BGR triplets

Both headers are declared as structs with `#pragma pack(push,1)`. Without it the compiler adds padding and the file header would be 16 bytes instead of the required 14, which corrupts the output.

### Row padding

Each row in a BMP must be a multiple of 4 bytes. The row size is computed with a bitwise trick:

```c
int rowSize = (width * 3 + 3) & ~3;
```

Adding 3 and then masking with `~3` (`...11111100`) rounds up to the nearest multiple of 4. The padding bytes are read and written along with the pixels so the output stays valid.

### Algorithms

- **Grayscale**: `gray = 0.114*B + 0.587*G + 0.299*R`, written to all three channels.
- **Invert**: each channel becomes `255 - value`.
- **Brightness**: each channel is adjusted by `value * percentage`, then clamped to `[0, 255]`.
- **Blur**: each interior pixel becomes the average of the 9 pixels in its 3x3 neighbourhood. The whole image is held in memory so that already-blurred pixels don't affect their neighbours. The 1-pixel border is left unchanged.
- **Crop**: updates the width, height, image size and file size in the headers, then copies the selected rows with fresh padding.
- **Rotate 180**: pixel `(x, y)` is moved to `(width-1-x, height-1-y)`.
- **Rotate 90**: width and height are swapped in the headers (and the row size recalculated), and each pixel is moved to its new position in a freshly allocated buffer.

## Limitations

- Only **24 bits per pixel, uncompressed** BMPs are supported. Palette-based (8-bit) and 32-bit files are not handled.
- Images with a negative height (top-down storage) are not specifically handled.
- BMP rows are stored bottom-up, so for `crop` the `y` coordinate is counted from the bottom row of the image.
- Basic argument validation only: the input file is opened before the argument count is checked, so running the program without arguments will crash.
