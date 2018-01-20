---
title: About
layout: default
---
Welcome to snestistics!

If you find any errors on this home page or in snestistics itself, feel free to [open an issue on GitHub](https://github.com/breakin/snestistics/issues) or to reach out to me on [Twitter](https://twitter.com/anders_breakin).

* Binaries: [https://github.com/breakin/snestistics/releases](https://github.com/breakin/snestistics/releases)
* Source: [https://github.com/breakin/snestistics](https://github.com/breakin/snestistics)

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
* Windows only (snestistics builds on windows/osx/linux but snes9x-snestistics only on windows)
* Only supports "LoROM" games.
* Non-CPU->CPU DMA not currently supported
	* Also DMA not supported at all by rewind
* Emulator has bugs; if you are working on a game and want to try snestistics, just ask me and I (breakin) will help you fix the emulation errors for that particular game. This helps me prioritize things that are actually needed by someone.
* Doesn't know about extension cartridges but it might work. Ask me if you need this.
* No support for games where code is executed from RAM (bank $7e or $7f)
* No emulation of PPU memory
* No emulation of SRAM (affects ability to inspect SRAM memory in trace log)
* Aloso see [bugs on github](https://github.com/breakin/snestistics/labels/bug)

Snestistics is far from ready but instead of keeping it a secret until it is properly finished (no bugs and all games working) I'm trying a different approach by putting it up as it is. The reason is that the audience for a tool like this is quite small so I rather give it out now and give priority to features/bugs that are affecting actual users! That way more people can enjoy snestistics. If you [file an issue](https://github.com/breakin/snestistics/issues) I will try to fix the bugs that are blocking you - an actual user - or at least give them higher priority.

Claim to Fame
=============
* Used in the [Super Famicom Wars translation project](https://www.romhacking.net/translations/3354/)

Contributors
============
* snestistics is written by Anders "breakin" Lindqvist.
* David "optiroc" Lindecrantz has been a huge source of ideas and motivation and has also helped greatly with this homepage (including co-authoring the tutorial and writing CSS)
* Huge inspiration was drawn from the snes9x code base, both the possibility to build/change/use it but also in looking at the source to understand Super Nintendo quirks

For a full history of snestistics see the [history](history) page.

Contact
=======
Feel free to engage with me at [Twitter](https://twitter.com/anders_breakin)!

NOTE: We will not provide ROMs. All ROMs we've used has been extracted from cartridges that we own.
