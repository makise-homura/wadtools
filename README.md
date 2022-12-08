# wadtools
Tiny tools for creating and unpacking WAD files under Linux

## Build

### Building

    gcc wadbuild.c  -o wadbuild
    gcc wadxtract.c -o wadxtract
    gcc wadpack.c -o wadpack
    gcc wadfindthing.c -o wadfindthing

Of course, you may build any tool you need instead of all.

### Installing

Set `$PREFIX` to, for example, `/usr` or `/usr/local`, or somewhat else.

    sudo cp wadbuild wadxtract wadpack wadfindthing $PREFIX/bin

## Invocation

### Create WAD file from separate lumps:

    wadbuild <wadfile> <filename> <lumpname> [<filename> <lumpname>...]

* `wadfile` is filename of newly created wadfile (will be overwritten if exist);

* `filename` is filename of lump to add (specify '-' if you want a zero-sized marker);

* `lumpname` is name of lump as which this file would reside in WAD.

Example:

    wadbuild mywad.wad \
             decorate1.txt DECORATE \
             decorate2.txt DECORATE \
             - A_START \
             acslib_compiled.o ACSLIB \
             - A_END \
             acslib.acs ACSLIB

This creates `mywad.wad` with the following entries:

* `DECORATE` - with `decorate1.txt` contents;

* `DECORATE` - with `decorate2.txt` contents;

* `A_START` - empty marker;

* `ACSLIB` - compiled ACS script from file `acslib_compiled.o`;

* `A_END` - empty marker;

* `ACSLIB` - ACS script source from file `acslib.acs`.

### Extract WAD file:

    wadxtract <wadfile> <directory>


* `wadfile` - filename of wadfile to extract data from;

* `directory` - path where to place resulting lumps (created if necessary; specify `.` to extract into current directory).

Each file name is prepended by number which indicates its position in WAD file directory, and `.lmp` extension is added.

Special characters unsafe for various file systems (`[`, `]`, `\` are replaced by `_`).

Example:

    wadxtract mywad.wad ./output

Extracts `mywad.wad` to `./output` directory.

Say, we have a file we created before, it will be decompressed to following files:

* `0001_DECORATE.lmp` - what has been known as `decorate1.txt` file;

* `0002_DECORATE.lmp` - what has been known as `decorate2.txt` file;

* `0003_A_START.lmp` - empty file;

* `0004_ACSLIB.lmp` - what has been known as `acslib_compiled.o` file;

* `0005_A_END.lmp` - empty file;

* `0006_ACSLIB.lmp` - what has been known as `acslib.acs` file.

### Pack WAD file back:

    wadpack <wadfile> <directory>


* `wadfile` - filename of newly created wadfile (will be overwritten if exist);

* `directory` - path from where to read extracted lumps (specify `.` to read from the current directory).

The only files to be packed are those whose names start with four digits followed by underscore. Lump name will inherit file name part starting with 6th character. Any part after `.` character is ignored, as well as any characters that could make lump name longer than 8 bytes.

Note: Special characters (`[`, `]`, `\`) are not restored! These lumps will have `_` in their name inside the WAD file.

Example:

    wadpack mywad.wad ./output

Builds `mywad.wad` from `./output` directory that was created by `wadxtract` call.

### Find a THING inside some map of WAD file:

    wadfindthing <wadfile> <version> <thing type>

* `wadfile`  - filename of wadfile to look for things in;

* `version` - expected version of THINGS lump: d for DOOM, h for Hexen;

* `thing type` - decimal number of thing type.

Example:

    wadfindthing mywad.wad h 9058

This looks for thing with type 9058 inside every map of `mywad.wad` which contains maps in Hexen format.
