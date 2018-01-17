---
title: About
layout: default
---
Welcome to snestistics. The source code can be at [GitHub](https://github.com/breakin/snestistics). If you find any errors on this home page or in snestistics itself, feel free to [open an issue on GitHub](https://github.com/breakin/snestistics/issues) or to reach out to me on [Twitter](https://twitter.com/anders_breakin).

Features
========
* Generate assembler source
* Cool data report (DMA, data reads...)
* Trace log with functions names from annotations
	* Ability to log function parameters from a script written in a squirrel script file. Full access to registers and RAM.
* Rewind report to trace execution flow backward in time

See [user-guide](user-guide) for a description on how to use the features!

Current Limitations
===================
* For now you need to build snestistics yourself (works on windows, osx and linux using cmake). An pre-build binary of the emulator for Windows can be found in /deps in the source code.
* Only supports "LoROM" games.
* Non-CPU->CPU DMA not currently supported
	* Also DMA not supported at all by rewind
* Emulator has bugs; if you are working on a game and want to try snestistics, just ask me and I (breakin) will help you fix the emulation errors for that particular game. This helps me prioritize things that are actually needed by someone.
* Doesn't know about extension cartridges but it might work. Ask me if you want to try this.
* No support for games where code is executed from RAM (bank $7e or $7f)
* No emulation of PPU memory
* See [bugs on github](https://github.com/breakin/snestistics/labels/bug)

Claim to Fame
=============
* Used in the [Super Famicom Wars translation project](https://www.romhacking.net/translations/3354/)

Contributors
============
* snestistics is written by Anders "breakin" Lindqvist.
* David "optiroc" Lindecrantz has been a huge source of ideas and motivation and has also helped greatly with the tutorial series on this page.
* Huge inspiration was drawn from the snes9x code base, both the possibility to build/change/use it but also in looking at the source to understand Super Nintendo quirks

For a full history of snestistics see the [history](history) page.

Contact
=======
Feel free to engage with me at [Twitter](https://twitter.com/anders_breakin)!

NOTE: We will not provide ROMs. All ROMs we've used has been extracted from cartridges that we own.
