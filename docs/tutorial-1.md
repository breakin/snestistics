---
---
Introduction
============
In order to do the SFW (Super Famicon Wars)  [Translation](http://www.romhacking.net/translations/3354) we had to start by understanding how to game worked. Once this was done an equally hard task was to change the game. But understanding came first. In order to understand we needed to reverse engineer! The tool [snestistics](https://github.com/breakin/snestistics) is what we used (and modified!) to solve this task. See (https://github.com/breakin/snestistics/blob/master/HISTORY.md) for a discussion on why it works like it does. In this blog series we will simply show what you can do with snestistics and not compare it to other tools and approaches. When we started SFW snestistics was not very mature at all. A lot of tedious tasks we had to do by hand and writing a straightforward tutorial like this would not be possible!

I am assuming that if you actually want to understand code here in depth you will know some 65816 assembly and have a high-leve understanding of the SNES.

The Subject
===========
The game we are going to take a closer look at is the japanese game [Battle Pinball](https://www.youtube.com/watch?v=VKIM2FrK2zY).

While not sure we will actually attain anything specific with this game, one idea is to translate it into english. As you can see the intro and the level selector is in Japanese while in-game text seems to be a mix of english and japanese. When we were doing SFW we were in secret mode until we *knew* we could do it. This time we just start pulling on some threads in public to see where we end up. No promises!

Interlude
=========
While I own a physical copy of the game and have dumped it myself I am not allowed to share the ROM and also not allowed to share the source code for the ROM even though I've dissassemblied it myself. Thus you, the reader, will only see glimps and for the full picture (text really!) you will need to run snestistics on a ROM you've obtained yourself. Furthermore snestistics is constantly being updated/improved so the snippets shown here might not be fully accurate. I will try to update this blog series when major features changing output from the tools are being introduced. That is I want this tutorial to be a show-case of the latest version of snestistics.

Running snes-9x
================
The first step to obtain the disassembly is to use the special version of snes9x that has been patched to output a snestistics .trace file. Note that this only work under Windows for now.

First start our version of snes9x (snes9x-snestistics.exe). Note the _Snestistics_ menu right next to the help menu.

![Booting up our Snes9x](/images/tutorial-1/startup.png)

Now we load the _pinball battle_ ROM and let the game start. Here is an image from the intro:

![Intro for the game](/images/tutorial-1/intro.png)

Now when we are satisfied we select _Snestistics->Save trace and exit..._:

![Saving trace](/images/tutorial-1/saving-trace.png)

The trace can now be found in the _Roms_ directory and the files can look something like this:

![Locating trace](/images/tutorial-1/trace-file.png)

Note that traces currently end up in the _current working directory_ so if you start a movie playback in snes9x the traces will end up in the directory of the movie instead. This will probably be fixed in a future version of snes9x-snestistics.

Only the .trace-file is important and the other ones can be deleted.

In a future post we will give more tips'n'trics to make things even better with regards to trace file creation in snes9x. For more information on why we are using an emulator and why it happens to currently be snes9x see [here](https://github.com/breakin/snestistics/blob/master/HISTORY.md).

Running snestistics
===================
Once you have your trace file you are ready to generate the assembler source. We assume that you are running snestistics from a shell such as bash or cmd.exe and that you've gotten snestistics pre-built for your platform somehow. The minimal command line parameters to generate assembler output are (put them on one line, newlines here to make it easier to read!)
~~~~~~~~~~~~~~~~
snestistics
  -romfile battle_pinball.smc
  -tracefile trace0.trace
  -asmfile pinball.asm
  -reportfile pinball_report.txt
~~~~~~~~~~~~~~~~
This assumes that you are standing in the directory where you've placed _battle_pinball.smc_ and _trace0.trace_. The outputs will be _pinball.asm_ and _pinball_report.txt_. There might be other output files (depending on settings for our snes9x variant but they can be ignored. There are many more [options available](DOC COMMAND LINE) but lets concentrate on these for today.

It is very important to note here that some games will work flawlessly out of the box while some will simply cause an error here (or even a crash). If you want to work on a game and you encounter issues here, don't hesitate to contact us and we will fix the issue. There are some known limitations discussed in the [snestistics readme](https://github.com/breakin/snestistics/).

Since we re-emulate the CPU portion of the game session and our emulator is not very fast this can take quite some time, but the result is cached so all future runs will be fast. It should be noted that there will be some files created next to the trace file that contains this cached data. If they are deleted they will be regenerated. They have a version so if a new version of snestistics is downloaded they will also be regenerated.

The assembler source
====================

What is in the source code? 
---------------------------
Snestistics only knows about operations that were executed during the emulation sessions done previously in snes9x. So unless all buttons were pressed, all levels were played etc there will be gaps. This can be seen as a good things or a bad thing; if you only recorded the first second of the game and the created the trace file then only instructions used during the first second will be included. 

Ok enough about that. Lets look at some random source code to see what it looks like!

A line of code
--------------
First lets look at few lines:

~~~~~~~~~~~~~~~~
label_8083DF:
    /* mI 00 0000 8083DF E2 20       */ sep.B #$20

label_8083E1:
    /* mI 80 0000 8083E1 AD 12 42    */ lda.W REG_FLAGS                 ; H/V Blank Flags and Joypad Status - [DB=80]
    /* mI 00 0000 8083E4 29 01       */ and.B #$01
    /* mI 00 0000 8083E6 D0 F9       */ bne.B label_8083E1
    /* mI 00 0000 8083E8 E2 10       */ sep.B #$10
    /* mi 00 0000 8083EA A0 02       */ ldy.B #$02
    /* mi 00 0000 8083EC C2 20       */ rep.B #$20
    /* Mi 00 0000 8083EE A2 00       */ ldx.B #$00

label_8083F0:
    /* Mi 80 0000 8083F0 BD CA 02    */ lda.W $02CA,x                   ; 7E02CA [DB=80 X=0, 2]
    /* Mi 80 0000 8083F3 9D D4 02    */ sta.W $02D4,x                   ; 7E02D4 [DB=80 X=0, 2]
    /* Mi 80 0000 8083F6 BD 18 42    */ lda.W $4218,x                   ; REG_JOY1L Controller Port Data Registers (Pad 1 - Low) - [DB=80 X=0, 2]
    /* Mi 80 0000 8083F9 9D CA 02    */ sta.W $02CA,x                   ; 7E02CA [DB=80 X=0, 2]
    /* Mi 80 0000 8083FC 5D D4 02    */ eor.W $02D4,x                   ; 7E02D4 [DB=80 X=0, 2]
    /* Mi 80 0000 8083FF 3D CA 02    */ and.W $02CA,x                   ; 7E02CA [DB=80 X=0, 2]
    /* Mi 80 0000 808402 9D DE 02    */ sta.W $02DE,x                   ; 7E02DE [DB=80 X=0, 2]
    /* Mi 00 0000 808405 E8          */ inx
    /* Mi 00 0000 808406 E8          */ inx
    /* Mi 00 0000 808407 88          */ dey
    /* Mi 00 0000 808408 D0 E6       */ bne.B label_8083F0
    /* Mi 00 0000 80840A 60          */ rts
~~~~~~~~~~~~~~~~

First we have a label called _label_8083DF_. The number after the word _label_ shows the program counter needed to execute this code. In this case the snippets starts at offset $83FD in bank $80 (the $ here simply means that the number is written in hexadecimal form).

Then if we look at a line in the code we have:
~~~~~~~~~~~~~~~~
/* mi 00 0000 8083EC C2 20       */ rep.B #$20
~~~~~~~~~~~~~~~~

What does each piece mean?

* The lower-case m indicates that we have a 8-bit accumulator (memory flag is set).
* The upper-case I indicates that we have 16-bit index registers (index flag is not set).
* The 80 means that the data bank was 80 when we came here
* The 0000 means that the direct page was 0 when we came here
* The 8083DF means that it is located at program counter $8083DF (bank $80, address $83DF).
* C2 20 is the encoding of this opcode as bytes. This is what can be found in the ROM-file.
* rep.B #$20 means that we clear the processor status flags for the flags in the number $20.
	* Note how on the next line the accumulator is now 16-bit (upper-case M).
	* We will not teach 65816 assembly here!
* Bonus: In this case we are in bank $80 which is a mirror of bank $00. This means that Battle Pinball is using FastROM.

The indicators in the comment are not perfect. They reflect the values encountered during our emulated session in snes9x. Maybe in a longer session this code will be called in other ways. If there is an uncertainty a star will be printed instead indicating that there was multiple choices.

While we have no idea what this code does it seems that it access MMIO-mapped registers dealing with the joypad so it seems to access joypad data.

Labels for jumps
----------------
In our code snippet we have a jump in the form of a branch.
~~~~~~~~~~~~~~~~
    /* Mi 00 0000 808408 D0 E6       */ bne.B label_8083F0
~~~~~~~~~~~~~~~~
It is very convenient that we get the label name. That makes it easy to search for it in the assembly source.

Labels for indirect jumps
-------------------------
Some jumps are data-driven. That means that they either get an address from somewhere (memory or ROM) and then jumps to that address. Since that address might not be part of the ROM (or hard to find) it is hard to find when just looking at the source code. In _Snestistics_ it appears as a comment.
~~~~~~~~~~~~~~~~
label_80E744:
    /* *I 00 0000 80E744 C2 30       */ rep.B #$30
    /* MI 00 0900 80E746 A5 70       */ lda.B $70                       ; 7E0970 [DP=900]
    /* MI 00 0000 80E748 29 7F 00    */ and.W #$007F
    /* MI 00 0000 80E74B 0A          */ asl A
    /* MI 00 0000 80E74C AA          */ tax
    /* MI 00 0000 80E74D 7C 50 E7    */ jmp.W ($E750,x)
          ; label_80E76E [X=0000]
          ; label_80E79C [X=0002]
          ; label_80E7B6 [X=0004]
          ; label_80E7E4 [X=0006]
          ; label_80E7FE [X=0008]
          ; label_80E82C [X=000A]
          ; label_80E846 [X=000C]
          ; label_80E87E [X=000E]
          ; label_80E896 [X=0010]
          ; label_80E8AB [X=0012]
          ; label_80E8CD [X=0014]
          ; label_80E90C [X=0016]
~~~~~~~~~~~~~~~~

We can see that in this unknown code starting at _label_80E744_ there is a jump to the indirect address located in RAM-memory plus the X register. It would be hard to guess where this jump goes so debugging in an emulator might be the best chance. But since we already emulated it we know where the jump went in our emulated session.

Where does the program "start"?
-------------------------------
TODO: Add support to mark NMI and RESET and then show how it looks here.

Bonus
------------------------
If we look at the line
~~~~~~~~~~~~~~~~
/* Mi 80 0000 8083F0 BD CA 02    */ lda.W $02CA,x                   ; 7E02CA [DB=80 X=0, 2]
~~~~~~~~~~~~~~~~
we see that there are some extra stuff in the comment. It gives more information. In this case it says that this particular operation was executed with X=0 and X=2 with DB (data bank) $80. The number 7E02CA simply indicates that the base pointer is mapped to bank $7E where some of the RAM-memory is located. This is mostly to remind about how this variant of the op is working. If there had been more values of X it could look like this:

~~~~~~~~~~~~~~~~
/* MI 00 0000 808623 9F 00 22 7E */ sta.L $7E2200,x                 ; 7E2200 [X=<0, 4, ..., 200>]
~~~~~~~~~~~~~~~~

Here we can see that this line were run with X=0, X=4, X=8, X=12 and so on until X=200. The <> symbols indicate that we are dealing with a range. The difference between all the values in the range is the same. It can be deduced from the two first numbers.

This feature is powerful if you want to know what values of X, Y, DB or DP were used for this particular instruction. If the operation used indirect addressing the indirect base pointer will be written in the comment. If it is in ROM memory you might just have found an important table!

Formatting
----------
There are many command line switches to toggle things on/off. They can be nice if you want a more clean source code or if you need more information only sometimes. Regenerating the source code is very fast (supposed to be faster than one second) so fast to switch. See [command-line documentation](DOC COMMAND LINE).

Can the source code be compiled?
--------------------------------
The short story is that the source code is almost WLA-DX compatible and provided a correct header (something that can be given to snestistics using -asmheaderfile) it can almost be compiled OVER the ROM without causing a diff. A future post will revisit this topic and it will also discuss limitations.

Other limitations
=================
If the game uses self-modifying code (writing code dynamically to RAM and running from RAM) there assembly listing will most likely be confusing (or _snestistics_ might even crash). If you want to RE a game using SMC please contact us and we might be able to help you at least get source code for the non SMC-parts. Other parts of snestistics still provide value even if SMC is used.

Closing words
=============
That was all for today. In the next post we will look at how you can start annotating the source code once you start to discover what things do.

Read more blog posts on snestistics [here](/snestistics.html).
Snestistics can be found at [github](https://github.com/breakin/snestistics).

TBD before posting
==================
* https://github.com/breakin/snestistics/blob/master/HISTORY.md
* Manual command line options - link
* Make snestistics put vectors in asm output (NMI, RESET, ...)