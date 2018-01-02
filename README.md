Emulation guided disassembler for Super Nintendo games.
This project is currently being refurbished but you can test it if you want to if you are a Windows user.
A pre-built Windows version of the modified emulator can be found in the repository.

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

High-level overview
===================
Snestistics works by saving data while emulating a play session in an emulator.
Using this data snestistics itself can quickly generate assembler source. Annotations can be added to make the assembler source code more readable.
This gives a very quick iterative workflow where you can read the assembler source, add/change annotations and then re-generate the source.
It is aimed at helping user understand what happens on the Snes CPU.

Features
========
* Generate Assembler source
* Data report
* DMA report
* Trace log with functions names from annotations
	* Ability to log function parameters from a script written in a squirrel script file. Full access to registers and RAM.

Limitations
===========
* Only supports LoROM games.
* Non-CPU -> CPU DMA not currently supported. 
* Emulator has bugs; if you are working on a game and want to try snestistics, just ask me and I will help you fix the emulation errors for that particular game. This helps me prioritize things that are actually needed by someone.
* Doesn't know about extension cartridges but it might work. Ask me if you want to try this.

Why emulation?
--------------
In the beginning snestistics was quite simple. The emulator knew about how the Snes CPU works and ran the code. It simply saved down at what program counters operations existed at and also the flags that affects instruction sizes/interpretation. This was very simple and worked for all games. Later when adding features such as the trace log snestistics needed to be able to actually replay the stream of operations. That is it needed to know the order they were executed in. To support this - and some other cool upcoming features - a CPU-emulator was added to snestistics. It is not complete yet and some features are missing, but thanks to this it is now possible to do tracelogs, change annotations and then generate a new tracelog quickly without involving the emulator.

Wish list
=========
* HiROM games
* Full dma-support
* Emulate GPU memory (but not GPU output).

Success Stories
===============
Snestistics was re-started to be able to work on the translation of Super Famicon Wars. That translation was finished and snestitics was a huge help!

Contributors
============
* breakin
* optiroc. Tons of suggestions on how to improve assembler view.

While not active contributors I've had a lot of help from the snes9x source code. I hope to be able to contribute back somehow!

Contact
=======
Feel free to engage with me at [twitter](https://twitter.com/anders_breakin)!
NOTE: We will not provide ROMs. All ROMs we've used has been extracted from cartridges that we own.