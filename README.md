Emulation guided disassembler for Super Nintendo games. Aimed at helping user understand Snes games in order to change them!

It works by saving data during a emulator play session. Using this data snestistics can quickly generate assembler source. Annotations about functions and data can be added to make the assembler source code more readable. This enables a very quick iterative work-flow where you can read the assembler source, add/change annotations and then re-generate the source.

For more information see the project page at [https://github.com/breakin/snestistics](https://github.com/breakin/snestistics).

Building on Windows
===================
First get the source code (here using git bash):
~~~~~~
git clone https://github.com/breakin/snestistics
cd snestistics
git submodule init
git submodule update --recursive
~~~~~~
Then use cmake to generate a Visual Studio solution (or something else). I prefer pointing cmake-gui to the root-directory of cmake. Make sure you choose a build directory that you like; cmake will fill it with many files. I usually put it in snestistics-build next to snestistics.

Building in Visual Studio is straightforward and if you build the INSTALL-project snestistics.exe will be placed in the install directory you specified with cmake-gui.

A pre-built Windows version of the modified emulator can be found in the repository (in /deps).

Contributors
============
* breakin
* optiroc. Tons of suggestions on how to improve assembler view.

While not active contributors I've had a lot of help from the snes9x source code. I hope to be able to contribute back somehow!

Contact
=======
Feel free to engage with me at [twitter](https://twitter.com/anders_breakin)!
NOTE: We will not provide ROMs. All ROMs we've used has been extracted from cartridges that we own.