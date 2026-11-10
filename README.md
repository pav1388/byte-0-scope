# Byte-0-scope - binary data visualizer

[English](README.md) | [Русский](README.ru.md)

Interactive representation of arbitrary files as an image:
each byte (or bit, or multi-byte pixel) is a point on the canvas.
Helps to "see" the data structure: headers, palettes, sprites,
compressed blocks, padding.

Author: pav13.

Development: part of the code was prepared with the help of an AI assistant.

GitHub: https://github.com/pav1388/byte-0-scope

Forum: https://www.emu-land.net/forum/index.php/topic,93496.0.html

## Screenshots

<p align="center">
  <img src="docs/screenshot-main.png" alt="main" width="400">
</p>

## Features

- Opening files of any size (from kilobytes to gigabytes): mmap for large, `QByteArray` for small - switchable in settings
- Bit formats: 1 / 2 / 3 / 4 / 8 bpp with configurable bit order (MSB/LSB) and row padding
- Multi-byte formats: RGB, BGR, GBR, BRG, GRB, RBG, RGBA, ARGB, BGRA, ABGR, RGB565, BGR565, RGB555, ARGB1555, Integer 16/32/64, Float 32/64
- Palettes: 90+ built-in (grayscale, heat, viridis, plasma, rainbow, terminal, etc.)
- Configurable palettes: threshold, entropy bands, bit plane, zeros-ones, custom, "from file"
- Special palettes: byte frequency, repeats highlighting
- Zoom (1-32x), panning, panorama overview with thumbnail
- Tile rendering with LRU cache, prefetching of neighboring tiles, configurable tile size and cache limit
- Info panel with byte details (hex/dec/bin/ASCII) and pixel (for multi-byte formats)
- Tooltip with byte details near the cursor (enabled in settings)
- Hex view: bytes + ASCII + addresses
- Dec/Hex converter (0 … 0xFFFFFFFF) in the right panel
- Debug info panel (enabled in settings)
- Width modes: manual, auto (1:1), power of 2, 16:9, 4:3, "bytes per row", nearest "round", divisor ≤ N
- Region: start / end / length, selection by left-click on canvas, "length" mode, hex input mode
- PNG export in three modes: current visible area, full image 1:1, full image at current scale
- Comparison of two files via XOR with difference highlighting and difference percentage
- Horizontal and vertical mirroring
- Dark and light themes
- Recent files list, drag & drop, opening from command line
- "Training" mode: built-in demo file with hidden structure to be recovered by adjusting settings
- UI localization: Russian, English

### Drag & Drop

| Extension | Action |
|---|---|
| any file | open as data source |
| `.b0s` | open paired data file with display settings loaded |

If you drag two or more files - the first two are loaded into XOR comparison mode.

### Settings files

**Global settings** - `byte0scope-settings.ini`:

- theme, language, window geometry
- load mode (mmap / into memory)
- performance: tile size, cache limit, prefetch
- recent files, last folder
- default values for new files (bpp, bit order, palette, width, scale, etc.)
- byte tooltip (`byte_tooltip`), debug panel (`show_debug_info`)
- hex view state: visibility, height, bytes per line, font size

The file is stored next to the executable; if the directory is not writable -
in the user configuration directory.

**Display settings** - `<filename>.b0s`:

- bpp and bit / pixel order / padding
- palette and its parameters
- width mode, width, step
- scale
- region (offset / length / length_mode)
- horizontal and vertical mirrors

Format - JSON (UTF-8), stored next to the source file and picked up
automatically on open. Pixels are not duplicated in `.b0s` - the source
of truth is the data file itself.

### Controls

| Action | What it does |
|---|---|
| Left-click on pixel | lock the byte (second left-click - unlock) |
| Right-click + drag | panning |
| Wheel | zoom (except in "Fit to window" / "Auto zoom" modes) |
| Ctrl + wheel | row width; if necessary, automatically switches width mode to "manual" |
| Left-click in hex view | select byte and synchronize with canvas |
| Dragging the top edge of hex view | change panel height |
| Buttons "\|+\|" / "+\|" in Region panel | select start / end of region by clicking on canvas |
| Ctrl+O | open file |
| Ctrl+S | save `.b0s` next to file |
| Ctrl+L | load `.b0s` |
| Ctrl+R | reset display settings |
| Ctrl+E | export to PNG (full image 1:1) |
| Ctrl+X | compare two files (XOR) |
| Ctrl+W | close current file |
| Drag & drop | open file by dragging |


### File associations

Menu "Settings":

- `Associate .b0s with this program` - `.b0s` opens in Byte-0-scope
- `Remove .b0s association`.

<details>
<summary><sub>.b0s format (JSON)</sub></summary>

Example:

```json
{
    "bpp_mode": 8,
    "bit_order": "msb",
    "pixel_order": "msb",
    "row_padding_bits": 0,
    "scale": 1,
    "width_mode": "fixed",
    "fixed_width": 256,
    "width_step": 8,
    "bytes_per_row": 16,
    "auto_scale": false,
    "fit_to_window": false,
    "offset": 0,
    "length": 0,
    "length_mode": false,
    "mirror_horizontal": false,
    "mirror_vertical": false,
    "palette": "heat",
    "palette_params": {
        "threshold": { "threshold": 64 }
    }
}
```

Unknown keys are ignored; missing ones are taken from defaults.
When opening a `.b0s` file as a command-line argument (or via association),
a data file without the `.b0s` suffix is searched for nearby; if not found -
a warning is displayed.

</details>

## Building

C++11, static Qt (Core, Gui, Widgets), CMake ≥ 3.5.
Two build profiles are supported:

| Profile | Qt        | Toolchain             | Purpose                                 |
|---------|-----------|-----------------------|-----------------------------------------|
| `xp32`  | 5.6.3     | MinGW 4.9.2, 32-bit   | Windows XP compatibility                |
| `win64` | 5.15.2    | MinGW 8.1.0, 64-bit   | modern 64-bit Windows                   |

For each profile in `scripts\` there is an identical set of bat scripts
(`<profile>` - `xp32` or `win64`):

```
build-<profile>.bat                incremental build -> byte-0-scope.exe
clean-build-<profile>.bat          delete build\ and rebuild
release-<profile>.bat              build releases\byte-0-scope-v<ver>\
translations-update-<profile>.bat  update .ts (lupdate) and build .qm
translations-release-<profile>.bat translations ready for release
```

`scripts\_build-menu.bat` - menu for launching these actions.

Directories with static Qt builds (`qt-5.6.3-static\`, `qt-5.15.2-static\`)
are placed in the repository root and are not included in git. How to build them - see
`scripts/BUILD-QT-5.6.3-static.md` and `scripts/BUILD-QT-5.15.2-static.md`.

Manual build (example for win64; for xp32 the directory names change):

```bat
cmake -S . -B build-win64 -G "MinGW Makefiles" ^
      -DCMAKE_PREFIX_PATH=qt-5.15.2-static ^
      -DBS_QT_DIR=qt-5.15.2-static ^
      -DCMAKE_BUILD_TYPE=Release
cmake --build build-win64 -j4
```

## Repository structure

```
CMakeLists.txt          build
version.txt             version (used in APP_VERSION and release-*.bat)
src/                    sources (C++/Qt)
src/resources/          .qrc and .rc (icons, translations)
scripts/                bat scripts for build, release and translations
scripts/_build-menu.bat build menu
scripts/BUILD-QT-*.md   how to build the static Qt used
```

## License

GPL-3.0-or-later. See [LICENSE](LICENSE).

The license was chosen because the binary is statically linked with Qt built
under GPL (`-opensource`).