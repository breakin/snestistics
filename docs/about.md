---
title: About
layout: default
---
Contributors
============
* Snestistics is written by Anders "breakin" Lindqvist.
* David "optiroc" Lindecrantz has been a huge source of ideas and motivation and has also helped greatly with the tutorial series on this page.
* Huge inspiration was drawn from the snes9x code base, both the possibility to build/change/use it but also in looking at the source to understand Super Nintendo quirks

For a full history of snestistics see the [history](history) page.

Features
========
* Generate Assembler source
* Data report
* DMA report
* Trace log with functions names from annotations
	* Ability to log function parameters from a script written in a squirrel script file. Full access to registers and RAM.
* Rewind report to trace execution flow backward in time

Current Limitations
===================
* Only supports LoROM games.
* Non-CPU->CPU DMA not currently supported
	* Also DMA not supported at all by rewind
* Emulator has bugs; if you are working on a game and want to try snestistics, just ask me and I (breakin) will help you fix the emulation errors for that particular game. This helps me prioritize things that are actually needed by someone.
* Doesn't know about extension cartridges but it might work. Ask me if you want to try this.
* No support for games where code is executed from RAM (bank $7e or $7f)
* No idea how it will work if there are extension cartridges!

Wish list
=========
* HiROM games
* Full dma-support
* Emulate PPU memory (but not the PPU itself)

Contact
=======
Feel free to engage with me at [twitter](https://twitter.com/anders_breakin)!
NOTE: We will not provide ROMs. All ROMs we've used has been extracted from cartridges that we own.