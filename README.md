yaqoi
=====

> Yet Another [QOI](https://qoiformat.org/) implementation in Zig.

## Usage

Clone the repository with its submodules ([zig-clap](https://github.com/Hejsil/zig-clap) and [stb](https://github.com/nothings/stb)) and on the right branch:

```
$ git clone --recurse-submodules -b zig https://github.com/190n/yaqoi.git
```

Compile with `zig build` (or add `-Drelease-safe` or `-Drelease-fast` if you feel like it).

Then the executable will be in `zig-out/bin`. Options:

```
    -h, --help
            display help

    -l, --linear-srgb
            indicate that output file is linear sRGB, not gamma (no actual conversion is done)

    -v, --verbose
            show encoding statistics

    -i, --input <filename>
            input file, default stdin

    -o, --output <filename>
            output file, default stdout

    -t, --threads <number>
            number of threads to use, default 1
```

## Status

The encoder works, and uses multiple threads, but it could be made faster. Currently with 8 threads in ReleaseFast it is about 2.5 times as fast as the _single-threaded_ C version.
