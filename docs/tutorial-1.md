---
layout: post
date: 2017-12-27
title: Snestistics Tutorial - Getting started
tags: snes, snestistics
theme: snestistics
landing: drafts
---
Introduction
============
In order to do the SFW (Super Famicon Wars)  [Translation](http://www.romhacking.net/translations/3354) we had to start by understanding how to game worked. Once this was done an equally hard task was to change the game. But understanding came first. In order to understand we needed to reverse engineer! The tool [snestistics](https://github.com/breakin/snestistics) is what we used (and modified!) to solve this task. See (https://github.com/breakin/snestistics/blob/master/HISTORY.md) for a discussion on why it works like it does. In this blog series we will simply show what you can do with snestistics and not compare it to other tools and approaches. When we started SFW snestistics was not very mature at all. A lot of tedious tasks we had to do by hand and writing a straightforward tutorial like this would not be possible!

I am assuming that if you actually want to understand code here in depth you will know some 65816 assembly and have a high-leve understanding of the SNES.

The Subject
===========
The game we are going to take a closer look at is the japanese game Battle Pinball.

![Battle Pinball in action](https://www.youtube.com/watch?v=VKIM2FrK2zY)

While not sure we will actually attain anything specific with this game, one idea is to translate it into english. As you can see the intro and the level selector is in Japanese while in-game text seems to be a mix of english and japanese. When we were doing SFW we were in secret mode until we *knew* we could do it. This time we just start pulling on some threads in public to see where we end up. No promises!

Interlude
=========
While I own a physical copy of the game and have dumped it myself I am not allowed to share the ROM and also not allowed to share the source code for the ROM even though I've dissassemblied it myself. Thus you, the reader, will only see glimps and for the full picture (text really!) you will need to run snestistics on a ROM you've obtained yourself. Furthermore snestistics is constantly being updated/improved so the snippets shown here might not be fully accurate. I will try to update this blog series when major features changing output from the tools are being introduced. That is I want this tutorial to be a show-case of the latest version of snestistics.

Running snes-9x
================
The first step to obtain the disassembly is to use the special version of snes9x that has been patched to output a snestistics .capture file.

TODO: Here I think we should have images from doing this.

1. Load the game
2. Start playing
3. Exit the game
4. Locate the .capture file

In a future post we will give more tips'n'trics to make things even better.
For more information on why we are using an emulator and why it happens to be snes9x see [here](https://github.com/breakin/snestistics/blob/master/HISTORY.md).

Running snestistics
===================
Once you have your trace file you are ready to generate the assembler source. We assume that you are running snestistics from a shell such as bash or cmd.exe and that you've gotten snestistics pre-built for your platform somehow. The minimal command line parameters to generate assembler output are
~~~~~~~~~~~~~~~~
snestistics
  -romfile battle_pinball.smc
  -tracefile capture1.trace
  -asmfile pinball.asm
  -labelsfile "[snestistics-directory]/data/hardware_reg.labels"
  -reportfile pinball_report.txt
~~~~~~~~~~~~~~~~
This assumes that you are standing in the directory where you've placed _battle_pinball.smc_ and _capture1.trace_. The outputs will be _pinball.asm_ and _pinball_report.txt_. There might be other output files (depending on settings for our snes9x variant9 but they can be ignored. There are many more [options available](DOC COMMAND LINE) but lets concentrate on these for today.

It is very important to note here that some games will work flawlessly out of the box while some will simply cause an error here (or even a crash). If you want to work on a game and you encounter issues here, don't hestiate to contact us and we will fix the issue. There are some known limitations discussed in the [snestistics readme](https://github.com/breakin/snestistics/).

Since we re-emulate the CPU portion of the game session and our emulator is not very fast this can take quite some time, but the reuslt is cached so all future runs will be fast. It should be noted that there will be some files created next to the trace file that contains this cached data. If they are deleted they will be regenerated. They have a version so if a new version of snestistics is donwloaded they will also be regenerated.

The assembler source
====================
First lets look at some random source code to see what it looks like.

A line of code
--------------
TODO: Find a label and then some code... talk about what the different parts are (PB, DB, ...).

Where does the program "start"?
-------------------------------
TODO: Add support to mark NMI and RESET and then show how it looks here.

Labels for jumps
----------------
TODO.

Indirect labels (jump tables)
-----------------------------
TODO.

TODO: What else for now?
------------------------
* DB, Mm Ii etc... finns massor av flaggor som är coola.. vad vill vi visa nu?

Formatting
----------
There are many command line switches to toggle things on/off. They can be nice if you want a more clean source code or if you need more information only sometimes. Regenerating the source code is very fast (supposed to be faster than one second) so fast to switch. See [command-line documentation](DOC COMMAND LINE).

What is in the source code? 
---------------------------
Snestistics only knows about operations that were executed during the emulation sessions done previously in snes9x. So unless all buttons were pressed, all levels were played etc there will be gaps. This can be seen as a good things or a bad thing; if you only recorded the first second of the game and the created the trace file then only instructions used during the first second will be included. 

Can the source code be compiled?
--------------------------------
The short story is that the source code is almost WLA-DX compatible and provided a correct header (something that can be given to snestistics using -asmheaderfile) it can almost be compiled OVER the ROM without causing a diff. A future post will revisit this topic and it will also discuss limitations.

Closing words
=============
That was all for today. In the next post we will look at how you can start annotating the source code once you start to discover what things do.

Read more blog posts on snestistics [here](/snestistics.html).
Snestistics can be found at [github](https://github.com/breakin/snestistics).

Bakläxa
=======
* https://github.com/breakin/snestistics/blob/master/HISTORY.md - Varför snes9x, varför emulering?
* Manual command line options - länk
* It feels like there is some confusion. Should it be tracefile? .trace? Is it called a capture? Change in snestistics once we have a good name
	* I think the woring should be that a trace is something that you can make full emulation from... A capture should probably be removed as word
	* If we re-introduce that the emulator can emulate for us 100% then we need a new name for that.
* Det känns fel att hw-regs kommer via labels som man måste addera själv, när de är oändringsbara (pga hw-register) och när de går via data-konceptet som nog måste bytas ut iaf... något vi ska fixa? Känns som något som kommer ge alla problem...
* Gör så att snestistics annoterarar upp NMI, RESET etc... Det ska stå i koden.