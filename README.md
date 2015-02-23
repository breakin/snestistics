snestistics
===========
Emulation guided dissassembler for snes games.

The "trick" is that the emulator says what is code and what is data. It also remembers all data-dependant jumps, as well as process mode (8-bit or 16-bit?) for each executed instruction. Snestistics then can make a source file where information from the emulator is incorporated, resulting in source code that might look like the original source code (with comments, labels etc). Perfect for reverse-engineering.

Tries to create a well-formed .ASM-file that can be compiled as a patch over the ROM-file using WLA DX. Code locations and jump targets are extracted using an emulator.

I've chosen to create a simple binary file format that any emulator can create. Currently I have a version of snes9x that supports this. See https://github.com/breakin/snes9x-debugtrace.
Going with output listing from Geigers snes9x might also be possible but not used atm.

Notice:
* I will not distribute any ROM-files.

I haven't decided on a license for this project, and currently I feel more like working on it, but feel free to try it out, fork it, use and whatever. Notice though that the two tables in snesops.h are borrowed from snes9x and I will have to fix my licenses for those!
