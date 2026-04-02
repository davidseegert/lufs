# LUFS - Loudness Analyzer

`lufs` is a high-performance C++ command-line tool for analyzing the loudness of audio files. It provides Integrated Loudness (LUFS/LKFS) and Loudness Range (LRA) measurements following international broadcasting standards.

## Features

- **Standard Compliant**:
  - **ITU-R BS.1770-5**: Integrated Loudness (LUFS) using K-weighting and dual-gating.
  - **EBU Tech 3342**: Loudness Range (LRA) using 3-second statistical windows.
- **Recursive Scanning**: Process entire directories and subdirectories automatically.
- **Versatile Decoding**: Powered by `miniaudio`, supporting WAV, MP3, FLAC, and more.
- **Aligned Formatting**: Clean, readable output with aligned values.

## Usage

Run `lufs` followed by a file or directory path. You can use optional flags to customize the output:

- `-l`: Output only the Integrated Loudness (LUFS).
- `-r`: Output only the Loudness Range (LRA).
- `-h`: Show help and usage information.

### Examples

**Default Output (LUFS & LRA)**
```bash
$ ./lufs track.mp3
track.mp3
LUFS: -14.2
LUR:    3.2

$ ./lufs music_folder/
file1.wav
LUFS: -16.5
LUR:    4.1

sub/file2.mp3
LUFS: -12.1
LUR:    2.1
```

**LUFS Only (`-l`)**
```bash
$ ./lufs -l track.mp3
-14.2

$ ./lufs -l music_folder/
file1.wav -16.5
sub/file2.mp3 -12.1
```

**Loudness Range Only (`-r`)**
```bash
$ ./lufs -r track.mp3
3.2

$ ./lufs -r music_folder/
file1.wav 4.1
sub/file2.mp3 2.1
```

## Installation

### Prerequisites
- A C++ compiler supporting C++17 (e.g., GCC or Clang).
- `make` build tool.

### Build
Simply run `make` in the project directory:
```bash
make
```
This will create the `lufs` executable.

## Technical Details
- **Language**: C++17
- **Core Library**: [miniaudio](https://github.com/mackron/miniaudio) (Single-header audio decoding)
- **Algorithm**: Implemented from scratch based on ITU-R BS.1770-5 specification.
- **Backends**: Uses `MA_NO_DEVICE_IO` to remain a lightweight, dependency-free CLI tool.

## References
- **[ITU-R BS.1770-5](https://www.itu.int/rec/R-REC-BS.1770/en)**: Algorithms to measure audio programme loudness and true-peak audio level.
- **[EBU Tech 3342](https://tech.ebu.ch/publications/tech3342)**: Loudness Range: A measure to supplement loudness normalisation in accordance with EBU R 128.
- **[EBU R 128](https://tech.ebu.ch/publications/r128)**: Loudness normalisation and permitted maximum level of audio signals.

## License
Copyright 2026 David Seegert

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
