snestistics
===========
Emulation guided dissassembler for snes games.

Tries to create a well-formed .ASM-file that can be compiled as a patch over the ROM-file using WLA DX. Code locations and jump targets are extracted using an emulator.

I've chosen to create a simple binary file format that any emulator can create. Currently I have a version of snes9x that supports this. See https://github.com/breakin/snes9x-debugtrace.

Notice:
* I will not distribute any ROM-files.
* Some code in source/snesops.h is stolen from snes9x.

I haven't decided on a license for this project, and currently I feel more like working on it, but feel free to try it out, fork it, use and whatever. Notice though that the two tables in snesops.h are borrowed from snes9x and I will have to fix my licenses for those!