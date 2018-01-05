---
title: Tutorial 2: Annotations (WIP)
---
Introduction
============
So we have our assembler source. Now starts a cycle of reading assembler, understanding _something_ (small or big) and then to annotate the assembler source so we don't have to remember it all. In snestistics we have a file called a labels file where annotations can be added. Let's revisit the function from the first [tutorial](tutorial-1) that interacted with the joypad:

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

We hand-craft the following annotations and put them in the file _tutorial2.labels_.
~~~~~~~~~~
@ Preliminary functions. This line is only to make the .labels-file easier to work with.
@ Good place to add TODO that you can search for.
; This function reads from the joypad
; We can write as many comment lines here as we want and they will appear before the function
# Reads joypad status
function 8083DF 80840A JoyPad1
comment 808405 "Here x is increased!"
function 8082CC 808304 Unknown1
; This code is special so let's add a label to make it more visible!
label 808407 ExtraLabel
~~~~~~~~~~

We run snestistics again with a new command line option so it used our annotation file:
~~~~~~~~~~~~~~~~
snestistics
  -romfile battle_pinball.sfc
  -tracefile trace0.trace
  -asmfile pinball.asm
  -reportfile pinball_report.txt
  -labelsfile tutorial2.labels
~~~~~~~~~~~~~~~~

This regenerates the assembler source. Let's take a look at it now:
~~~~~~~~~~
; This function reads from the joypad
; We can write as many comment lines here as we want and they will appear before the function
JoyPad1:
    /* mI 00 0000 8083DF E2 20       */ sep.B #$20

_JoyPad1_8083E1:
    /* mI 80 0000 8083E1 AD 12 42    */ lda.W REG_FLAGS                 ; H/V Blank Flags and Joypad Status - [DB=80]
    /* mI 00 0000 8083E4 29 01       */ and.B #$01
    /* mI 00 0000 8083E6 D0 F9       */ bne.B _JoyPad1_8083E1
    /* mI 00 0000 8083E8 E2 10       */ sep.B #$10
    /* mi 00 0000 8083EA A0 02       */ ldy.B #$02
    /* mi 00 0000 8083EC C2 20       */ rep.B #$20
    /* Mi 00 0000 8083EE A2 00       */ ldx.B #$00

_JoyPad1_8083F0:
    /* Mi 80 0000 8083F0 BD CA 02    */ lda.W $02CA,x                   ; 7E02CA [DB=80 X=0, 2]
    /* Mi 80 0000 8083F3 9D D4 02    */ sta.W $02D4,x                   ; 7E02D4 [DB=80 X=0, 2]
    /* Mi 80 0000 8083F6 BD 18 42    */ lda.W $4218,x                   ; REG_JOY1L Controller Port Data Registers (Pad 1 - Low) - [DB=80 X=0, 2]
    /* Mi 80 0000 8083F9 9D CA 02    */ sta.W $02CA,x                   ; 7E02CA [DB=80 X=0, 2]
    /* Mi 80 0000 8083FC 5D D4 02    */ eor.W $02D4,x                   ; 7E02D4 [DB=80 X=0, 2]
    /* Mi 80 0000 8083FF 3D CA 02    */ and.W $02CA,x                   ; 7E02CA [DB=80 X=0, 2]
    /* Mi 80 0000 808402 9D DE 02    */ sta.W $02DE,x                   ; 7E02DE [DB=80 X=0, 2]
    /* Mi 00 0000 808405 E8          */ inx
    /* Mi 00 0000 808406 E8          */ inx

; This code is special so let's add a label to make it more visible!
_JoyPad1_ExtraLabel:
    /* Mi 00 0000 808407 88          */ dey
    /* Mi 00 0000 808408 D0 E6       */ bne.B _JoyPad1_8083F0
    /* Mi 00 0000 80840A 60          */ rts
~~~~~~~~~~

So that looks nicer! As you investigate the source comments can be added and changed. Since the assembler source itself is never edited you never end up in situations where you feel that you should "refactor" it or something. The entire structure of the assembler source can be changed just by changing the command line options to snestistics. We could even en theory target different assembler dialects!

Are there any other benefits to adding annotations besides comments? I'm glad you asked!

Jumps to annotated function
---------------------------
So let's search for the word JoyPad1 in the generated assembly source. We only get one hit (not counting the annotated function itself):
~~~~~~~~~~
label_8082C5:
    /* *I 00 0000 8082C5 20 CC 82    */ jsr.W label_8082CC
    /* mI 00 0000 8082C8 20 DF 83    */ jsr.W JoyPad1                   ; Reads joypad status
    /* Mi 00 0000 8082CB 60          */ rts
~~~~~~~~~~

We see that there is a label here which means someone jumps here. Then _label_8082CC_ is called and then _JoyPad1_. We see how the name _JoyPad1_ here is written instead of the address of the jump, and in the comment we can see the #-line we added when annotating the function. We call the #-comment the "use comment" and it is printed whenever the name of the function is used.

Before annotating we can of course just search for the label-name itself. The real value is in assigning good names, names that improves as your understanding of the game improves.

Without annotations the code at 8082C5 looked like this:
~~~~~~~~~~
label_8082C5:
    /* *I 00 0000 8082C5 20 CC 82    */ jsr.W label_8082CC
    /* mI 00 0000 8082C8 20 DF 83    */ jsr.W label_8083DF
    /* Mi 00 0000 8082CB 60          */ rts
~~~~~~~~~~

It would be much harder to know that it had something to do with JoyPad1! But what does _label_8082CC_ do? Let's look at it:
~~~~~~~~~~
label_8082CC:
    /* *I 00 0000 8082CC C2 10       */ rep.B #$10
    /* *I 00 0000 8082CE E2 20       */ sep.B #$20
    /* mI 00 0000 8082D0 A0 00 00    */ ldy.W #$0000

label_8082D3:
    /* mI 80 0000 8082D3 BE 05 83    */ ldx.W $8305,y                   ; 808305 [DB=80 Y=<0, 4, ..., DC>]
    /* mI 00 0000 8082D6 E0 FF FF    */ cpx.W #$FFFF
    /* mI 00 0000 8082D9 F0 0F       */ beq.B label_8082EA
    /* mI 80 0000 8082DB BD 00 00    */ lda.W $0000,x                   ; 7E0000 [DB=80 X=<292, 293, ..., 2C8>]
    /* mI 80 0000 8082DE BE 07 83    */ ldx.W $8307,y                   ; 808307 [DB=80 Y=<0, 4, ..., D8>]
    /* mI 80 0000 8082E1 9D 00 00    */ sta.W $0000,x                   ; 7E0000 [DB=80 X=2100, 2101, <2105, 2106, ..., 2115>, <211A, 211B, ..., 2121>, <2123, 2124, ..., 2132>]
    /* mI 00 0000 8082E4 C8          */ iny
    /* mI 00 0000 8082E5 C8          */ iny
    /* mI 00 0000 8082E6 C8          */ iny
    /* mI 00 0000 8082E7 C8          */ iny
    /* mI 00 0000 8082E8 80 E9       */ bra.B label_8082D3

label_8082EA:
    /* mI 00 0000 8082EA E2 20       */ sep.B #$20
    /* mI 80 0000 8082EC AD F0 02    */ lda.W $02F0                     ; 7E02F0 [DB=80]
    /* mI 00 0000 8082EF 09 20       */ ora.B #$20
    /* mI 80 0000 8082F1 8D 32 21    */ sta.W REG_COLDATA               ; Color Math Registers - [DB=80]
    /* mI 80 0000 8082F4 AD F1 02    */ lda.W $02F1                     ; 7E02F1 [DB=80]
    /* mI 00 0000 8082F7 09 40       */ ora.B #$40
    /* mI 80 0000 8082F9 8D 32 21    */ sta.W REG_COLDATA               ; Color Math Registers - [DB=80]
    /* mI 80 0000 8082FC AD F2 02    */ lda.W $02F2                     ; 7E02F2 [DB=80]
    /* mI 00 0000 8082FF 09 80       */ ora.B #$80
    /* mI 80 0000 808301 8D 32 21    */ sta.W REG_COLDATA               ; Color Math Registers - [DB=80]
    /* mI 00 0000 808304 60          */ rts
~~~~~~~~~~

Well we don't know what it does (yet) and it is often hard to say without context, but what we can say is that it looks like a function. Let's add an annotation for it just for fun in tutorial2.labels:

~~~~~~~~~~
function 8082CC 808304 Unknown1
~~~~~~~~~~

Our full block of code now looks like this:
~~~~~~~~~~
label_8082C5:
    /* *I 00 0000 8082C5 20 CC 82    */ jsr.W Unknown1
    /* mI 00 0000 8082C8 20 DF 83    */ jsr.W JoyPad1                   ; Reads joypad status
    /* Mi 00 0000 8082CB 60          */ rts

Unknown1:
    /* *I 00 0000 8082CC C2 10       */ rep.B #$10
    /* *I 00 0000 8082CE E2 20       */ sep.B #$20
    /* mI 00 0000 8082D0 A0 00 00    */ ldy.W #$0000

_Unknown1_8082D3:
    /* mI 80 0000 8082D3 BE 05 83    */ ldx.W $8305,y                   ; 808305 [DB=80 Y=<0, 4, ..., DC>]
    /* mI 00 0000 8082D6 E0 FF FF    */ cpx.W #$FFFF
    /* mI 00 0000 8082D9 F0 0F       */ beq.B _Unknown1_8082EA
    /* mI 80 0000 8082DB BD 00 00    */ lda.W $0000,x                   ; 7E0000 [DB=80 X=<292, 293, ..., 2C8>]
    /* mI 80 0000 8082DE BE 07 83    */ ldx.W $8307,y                   ; 808307 [DB=80 Y=<0, 4, ..., D8>]
    /* mI 80 0000 8082E1 9D 00 00    */ sta.W $0000,x                   ; 7E0000 [DB=80 X=2100, 2101, <2105, 2106, ..., 2115>, <211A, 211B, ..., 2121>, <2123, 2124, ..., 2132>]
    /* mI 00 0000 8082E4 C8          */ iny
    /* mI 00 0000 8082E5 C8          */ iny
    /* mI 00 0000 8082E6 C8          */ iny
    /* mI 00 0000 8082E7 C8          */ iny
    /* mI 00 0000 8082E8 80 E9       */ bra.B _Unknown1_8082D3

_Unknown1_8082EA:
    /* mI 00 0000 8082EA E2 20       */ sep.B #$20
    /* mI 80 0000 8082EC AD F0 02    */ lda.W $02F0                     ; 7E02F0 [DB=80]
    /* mI 00 0000 8082EF 09 20       */ ora.B #$20
    /* mI 80 0000 8082F1 8D 32 21    */ sta.W REG_COLDATA               ; Color Math Registers - [DB=80]
    /* mI 80 0000 8082F4 AD F1 02    */ lda.W $02F1                     ; 7E02F1 [DB=80]
    /* mI 00 0000 8082F7 09 40       */ ora.B #$40
    /* mI 80 0000 8082F9 8D 32 21    */ sta.W REG_COLDATA               ; Color Math Registers - [DB=80]
    /* mI 80 0000 8082FC AD F2 02    */ lda.W $02F2                     ; 7E02F2 [DB=80]
    /* mI 00 0000 8082FF 09 80       */ ora.B #$80
    /* mI 80 0000 808301 8D 32 21    */ sta.W REG_COLDATA               ; Color Math Registers - [DB=80]
    /* mI 00 0000 808304 60          */ rts
~~~~~~~~~~

We added hundreds if not thousands of these Unknown functions during reverse engineering of SFW! We will see later why annotated completely unknown function can help!

Indirect jumps
---------------
Let's add some more annotations in our labels-file:
~~~~~~~~~~
function 80E412 80E43B Trampoline
# Seems to be jump target
function 80E486 80E4A9 JumpFunction1
# Seems to be jump target
function 80E56E 80E5A7 JumpFunction2
~~~~~~~~~~

Our indirect jump example from the [first](tutorial-1) tutorial now looks like this:
~~~~~~~~~~
Trampoline:
    /* mI 00 0000 80E412 C2 30       */ rep.B #$30
    /* MI 00 0900 80E414 A5 70       */ lda.B $70                       ; 7E0970 [DP=900]
    /* MI 00 0000 80E416 29 7F 00    */ and.W #$007F
    /* MI 00 0000 80E419 0A          */ asl A
    /* MI 00 0000 80E41A AA          */ tax
    /* MI 00 0000 80E41B 7C 1E E4    */ jmp.W ($E41E,x)
          ; label_80E43C [X=0000]
          ; label_80E460 [X=0002]
          ; label_80E461 [X=0004]
          ; label_80E485 [X=0006]
          ; JumpFunction1 Seems to be jump target - [X=0008]
          ; label_80E4AA [X=000A]
          ; label_80E4AB [X=000C]
          ; label_80E4CF [X=000E]
          ; label_80E4D0 [X=0010]
          ; label_80E54E [X=0012]
          ; JumpFunction2 Seems to be jump target - [X=0014]
          ; label_80E5A8 [X=0016]
~~~~~~~~~~

This doesn't help us much. But if we actually investigated a function that we don't know how and why it is called (because address come from data) this is very helpful. It is all about revealing patterns! Remember that our real problem here is that we don't have the original source code and we know nothing about the game. At least for now!

What is a function?
===================
Our current concept is a function. We realize that since the games are not written in a high-level language that have consistent calling conventions this might not be a good fit for all games. This is an area where we are constantly trying to come up with new concepts for and each new game will probably push us to improve here.

Closing words
=============
Working iteratively with labels is extremely powerful. The auto-annotator is very fun to play around with and when it works it saved so much work.

[Here](code/tutorial2.labels) is the full labels file if you want to experiment with it.

TODO
====
* It seems as if line-comment are not working (at least not as shown here...)

<!-- Markdeep: --><style class="fallback">body{visibility:hidden;white-space:pre;font-family:monospace}</style><script src="markdeep.min.js"></script><script src="https://casual-effects.com/markdeep/latest/markdeep.min.js?"></script><script>window.alreadyProcessedMarkdeep||(document.body.style.visibility="visible")</script>
