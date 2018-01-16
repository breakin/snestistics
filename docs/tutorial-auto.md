---
title: Tutorial 5: Auto Annotations
layout: default
---
There are many small functions that does _something_. Often we don't need to know what they all do, but some of the tools in snestistics such as the trace log wants all program counters that are executed during a trace log session to be in annotated functions. When we did the SFW translation Anders did 1000s of functions manually (mostly since he wanted to help David out and he was procrastinating on the next feature for snestistics). Then one day he said enough and wrote an auto-detector that worked really well for SFW. It found an additional 500 functions and then there was no more manual work to be done.

We've tried it on other games with worse results and this is a feature I imagine will be rewritten a lot. When an *autolabelsfile* is specified auto annotation will run and the file will be saved. If that file is still part of the command line it will be reused, unless -autoannotate is specified. It is up to the user if he/she want the auto labels file to remain auto (by keeping it as a autolabelsfile) or if they simply want to treat it as a regular labels file after it has been generated. This enabled manual editing to fix up "broken" functions. We recommend putting it in a labels file of its own either way such that it can be regenerated if needed later.

Lets look at this feature then! Here is a command line:
~~~~~~~~~~~~~~~~
snestistics
  -romfile battle_pinball.sfc
  -tracefile trace0.trace
  -asmoutfile pinball.asm
  -predict everywhere
  -autolabelsfile auto.labels
~~~~~~~~~~~~~~~~~~~~~~~
See [user-guide](user-guide) for more command line options.

Lets see how it fares on _Battle Pinball_.

Before auto-annotate:
~~~~~~~~~~~~~~~~~~~~~~~
; Vector: Emulation RESET
label_008000:
    /* mi 00 0000 008000 78          */ sei
    /* mi 00 0000 008001 C2 09       */ rep.b #$09
    /* mi 00 0000 008003 FB          */ xce
    /* mi 00 0000 008004 5C 08 80 80 */ jmp.l label_808008

...

; Vector: Native NMI
label_0081B8:
    /* **         0081B8 5C BC 81 80 */ jmp.l label_8081BC

...

label_8081BC:
    /* **         8081BC C2 30       */ rep.b #$30
    /* MI         8081BE 48          */ pha
    /* MI         8081BF DA          */ phx
    /* MI         8081C0 5A          */ phy
    /* MI         8081C1 0B          */ phd
    /* MI         8081C2 8B          */ phb
    /* MI         8081C3 4B          */ phk
    /* MI         8081C4 AB          */ plb
    /* MI 80      8081C5 E2 20       */ sep.b #$20
    /* mI 80      8081C7 AD 10 42    */ lda.w REG_RDNMI                 ; NMI Enable - [DB=80]
    /* mI 80      8081CA 20 00 82    */ jsr.w label_808200
    /* *I 80      8081CD 20 C5 82    */ jsr.w label_8082C5
    /* Mi 80      8081D0 E2 20       */ sep.b #$20
    /* mi 80      8081D2 AD 87 02    */ lda.w $0287                     ; 7E0287 [DB=80]
    /* mi 80      8081D5 F0 1A       */ beq.b label_8081F1
    /*pmi 80 0000 8081D7 0D C9 02    */ ora.w $02C9
    /*pmi 80 0000 8081DA 8D C9 02    */ sta.w $02C9
    /*pmi 80 0000 8081DD 8D 00 42    */ sta.w $4200
    /*pmi 80 0000 8081E0 C2 10       */ rep.b #$10
    /*pmI 80 0000 8081E2 AE 88 02    */ ldx.w $0288
    /*pmI 80 0000 8081E5 29 20       */ and.b #$20
    /*pmI 80 0000 8081E7 D0 05       */ bne.b label_8081EE
    /*pmI 80 0000 8081E9 8E 07 42    */ stx.w $4207
    /*pmI 80 0000 8081EC 80 03       */ bra.b label_8081F1

label_8081EE:
    /*pmI 80 0000 8081EE 8E 09 42    */ stx.w $4209

label_8081F1:
    /* mi 80      8081F1 E2 30       */ sep.b #$30
    /* mi 80      8081F3 A9 FF       */ lda.b #$FF
    /* mi 80      8081F5 8D 90 02    */ sta.w $0290                     ; 7E0290 [DB=80]
    /* mi 80      8081F8 C2 30       */ rep.b #$30
    /* MI 80      8081FA AB          */ plb
    /* MI         8081FB 2B          */ pld
    /* MI         8081FC 7A          */ ply
    /* MI         8081FD FA          */ plx
    /* MI         8081FE 68          */ pla
    /* MI         8081FF 40          */ rti
~~~~~~~~~~~~~~~~~~~~~~~

After running snestistics with the command line above we get a auto.labels file that looks like this:
~~~~~~~~~~~~~~~~~~~~~~~
function 008000 008004 Auto0000
function 0081B8 0081B8 Auto0001
function 808008 80818F Auto0002
function 8081BC 8081FF Auto0003
function 808200 80820B Auto0004
...
function A28907 A2898F Auto0530
function A28990 A2899E Auto0531
~~~~~~~~~~~~~~~~~~~~~~~
That is 532 ranges that have been given a name of *AutoXXXX*. Lets see how the assembly looks now:
~~~~~~~~~~~~~~~~~~~~~~~
; Vector: Emulation RESET
Auto0000:
    /* mi 00 0000 008000 78          */ sei
    /* mi 00 0000 008001 C2 09       */ rep.b #$09
    /* mi 00 0000 008003 FB          */ xce
    /* mi 00 0000 008004 5C 08 80 80 */ jmp.l Auto0002

...

; Vector: Native NMI
Auto0001:
    /* **         0081B8 5C BC 81 80 */ jmp.l Auto0003

...

Auto0003:
    /* **         8081BC C2 30       */ rep.b #$30
    /* MI         8081BE 48          */ pha
    /* MI         8081BF DA          */ phx
    /* MI         8081C0 5A          */ phy
    /* MI         8081C1 0B          */ phd
    /* MI         8081C2 8B          */ phb
    /* MI         8081C3 4B          */ phk
    /* MI         8081C4 AB          */ plb
    /* MI 80      8081C5 E2 20       */ sep.b #$20
    /* mI 80      8081C7 AD 10 42    */ lda.w REG_RDNMI                 ; NMI Enable - [DB=80]
    /* mI 80      8081CA 20 00 82    */ jsr.w Auto0004
    /* *I 80      8081CD 20 C5 82    */ jsr.w Auto0007
    /* Mi 80      8081D0 E2 20       */ sep.b #$20
    /* mi 80      8081D2 AD 87 02    */ lda.w $0287                     ; 7E0287 [DB=80]
    /* mi 80      8081D5 F0 1A       */ beq.b _Auto0003_8081F1
    /*pmi 80 0000 8081D7 0D C9 02    */ ora.w $02C9
    /*pmi 80 0000 8081DA 8D C9 02    */ sta.w $02C9
    /*pmi 80 0000 8081DD 8D 00 42    */ sta.w $4200
    /*pmi 80 0000 8081E0 C2 10       */ rep.b #$10
    /*pmI 80 0000 8081E2 AE 88 02    */ ldx.w $0288
    /*pmI 80 0000 8081E5 29 20       */ and.b #$20
    /*pmI 80 0000 8081E7 D0 05       */ bne.b _Auto0003_8081EE
    /*pmI 80 0000 8081E9 8E 07 42    */ stx.w $4207
    /*pmI 80 0000 8081EC 80 03       */ bra.b _Auto0003_8081F1

_Auto0003_8081EE:
    /*pmI 80 0000 8081EE 8E 09 42    */ stx.w $4209

_Auto0003_8081F1:
    /* mi 80      8081F1 E2 30       */ sep.b #$30
    /* mi 80      8081F3 A9 FF       */ lda.b #$FF
    /* mi 80      8081F5 8D 90 02    */ sta.w $0290                     ; 7E0290 [DB=80]
    /* mi 80      8081F8 C2 30       */ rep.b #$30
    /* MI 80      8081FA AB          */ plb
    /* MI         8081FB 2B          */ pld
    /* MI         8081FC 7A          */ ply
    /* MI         8081FD FA          */ plx
    /* MI         8081FE 68          */ pla
    /* MI         8081FF 40          */ rti
~~~~~~~~~~~~~~~~~~~~~~~
As we can see in the case of *Auto0003* it is now very easy to differentiate between the start of a function and inner labels. We also have an easier time understanding that all the code of Auto0003 belong to one function. We also see here that [predicted code](tutorial-predict) is part of this function (the small p in the beginning of some lines).

Now it is high time we went back to our game and that is what we will do in the [next post](tutorial-re1).
