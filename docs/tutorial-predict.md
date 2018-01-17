---
title: Tutorial 4 - Code Prediction
layout: default
---
This tutorial post will go in-depth in the assembly listing to clean it up. Feel free to skip it or save it for later; it is optional!

By now you've might have looked a bit at the source code and you might have noticed something strange. Something annoying. Let's take a look.

In the generated assembler listing we can find short spurs of data in the middle of code. Something like this
~~~~~~~~~~~~~~~~
label_80821
    /* *I 80      80821E E2 20       */ sep.b #$20
    /* mI 80      808220 A9 80       */ lda.b #$80
    /* mI 80      808222 BC 00 01    */ ldy.w $0100,x
    /* mI 80      808225 F0 24       */ beq.b label_80824B
    /* mI 80      808227 10 01       */ bpl.b label_80822A

.DB $1A

label_80822A:
    /* mI 80      80822A 8D 15 21    */ sta.w REG_VMAIN
    /* mI 80      80822D C2 20       */ rep.b #$20
    /* MI 80      80822F 98          */ tya
    /* MI 80      808230 29 FF 00    */ and.w #$00FF
    /* MI 80      808233 A8          */ tay
    /* MI 80      808234 BD 02 01    */ lda.w $0102,x
    /* MI 80      808237 8D 16 21    */ sta.w REG_VMADDL
~~~~~~~~~~~~~~~~
What is that about? Well in this case there is code at the address after the instruction at 808227 but it was never run as part of our emulation session. Luckily the primitive prediction heuristic built into snestistics can mitigate this. Let's test a new command line option!
~~~~~~~~~~~~~~~~
snestistics
  -romfile battle_pinball.sfc
  -tracefile trace0.trace
  -asmoutfile pinball.asm
  -reportoutfile pinball_report.txt
  -predict everywhere
~~~~~~~~~~~~~~~~
The default for predict is *functions* which only predicts within annotated function. But sometimes the missing code can be annoying when you are annotating things since it can leave you unsure about some data before or after a function. Is it code? Is it data? Is it part of my function? Let's see what happened:
~~~~~~~~~~~~~~~~
label_80821E:
    /* *I 80      80821E E2 20       */ sep.b #$20
    /* mI 80      808220 A9 80       */ lda.b #$80
    /* mI 80      808222 BC 00 01    */ ldy.w $0100,x
    /* mI 80      808225 F0 24       */ beq.b label_80824B
    /* mI 80      808227 10 01       */ bpl.b label_80822A
    /*pmI 80 0000 808229 1A          */ inc A

label_80822A:
    /* mI 80      80822A 8D 15 21    */ sta.w REG_VMAIN
    /* mI 80      80822D C2 20       */ rep.b #$20
    /* MI 80      80822F 98          */ tya
    /* MI 80      808230 29 FF 00    */ and.w #$00FF
    /* MI 80      808233 A8          */ tay
    /* MI 80      808234 BD 02 01    */ lda.w $0102,x
    /* MI 80      808237 8D 16 21    */ sta.w REG_VMADDL
~~~~~~~~~~~~~~~~
The *p* before the instruction at 808229 means *predicted*. In this case snestistics just predicted that the bpl at 808227 sometime can not jump even though that didn't happen in this session. What happens when this assumption is faulty? From the report file *pinball_report.txt* we can find the following in the prediction report:
~~~~~~~~~~~~~~~~
Predicted jump at 81E914 jumped inside instruction at 81E8F7. Consider adding a "hint branch_always/branch_never 81E8F7" annotation.
Predicted jump at 81E932 jumped inside instruction at 81E907. Consider adding a "hint branch_always/branch_never 81E907" annotation.
Predicted jump at 818722 jumped inside instruction at 818730. Consider adding a "hint branch_always/branch_never 818730" annotation.
Predicted jump at 818746 jumped inside instruction at 818732. Consider adding a "hint branch_always/branch_never 818732" annotation.
Predicted jump at 81872D jumped inside instruction at 818764. Consider adding a "hint branch_always/branch_never 818764" annotation.
Predicted jump at 9DDED9 jumped inside instruction at 9DDEF5. Consider adding a "hint branch_always/branch_never 9DDEF5" annotation.
Predicted jump at 9DDEAA jumped inside instruction at 9DDEFF. Consider adding a "hint branch_always/branch_never 9DDEFF" annotation.
Predicted jump at 9DDF23 jumped inside instruction at 9DDF19. Consider adding a "hint branch_always/branch_never 9DDF19" annotation.
Predicted jump at 9FFF67 jumped inside instruction at 9FFEE7. Consider adding a "hint branch_always/branch_never 9FFEE7" annotation.
~~~~~~~~~~~~~~~~
This means that the prediction heuristic chickened out a few times because it seems like taking a jump led to a weird situation. Let's investigate the 3rd and 4th of these.

First we have a branch that is always taken in the emulated session. The predictor tries to not take it but all of the predicted ops looks like broken code:
~~~~~~~~~~~~~~~~
    /* mi 81 0B00 8186C0 D0 18       */ bne.b label_8186DA
    /*pmi 81 0B00 8186C2 A5 87       */ lda.b $87
    /*pmi 81 0B00 8186C4 4A          */ lsr A
    /*pmi 81 0B00 8186C5 EB          */ xba
    /*pmi 81 0B00 8186C6 A5 85       */ lda.b $85
    /*pmi 81 0B00 8186C8 4A          */ lsr A
    /*pmi 81 0B00 8186C9 C2 30       */ rep.b #$30
    /*pMI 81 0B00 8186CB 85 02       */ sta.b $02
    /*pMI 81 0B00 8186CD 8A          */ txa
    /*pMI 81 0B00 8186CE 0A          */ asl A
    /*pMI 81 0B00 8186CF 85 0E       */ sta.b $0E
    /*pMI 81 0B00 8186D1 64 0C       */ stz.b $0C

label_8186D3:
    /*pMI 81 0B00 8186D3 A6 0C       */ ldx.b $0C
    /*pMI 81 0B00 8186D5 BD 16 05    */ lda.w $0516,x
    /*pMI 81 0B00 8186D8 D0 02       */ bne.b label_8186DC
~~~~~~~~~~~~~~~~
and here
~~~~~~~~~~~~~~~~
label_8186DC:
    /*pmi 81 0B00 8186DC C9 05       */ cmp.b #$05
    /*pmi 81 0B00 8186DE 00 B0       */ brk.b $B0
    /*pmi 81 0B00 8186E0 03 4C       */ ora.b $4C,s
    /*pmi 81 0B00 8186E2 77 87       */ adc.b [$87],y
    /*pmi 81 0B00 8186E4 3A          */ dec A
    ...
~~~~~~~~~~~~~~~~
The *brk* instruction feels very weird. It could be debug code though so who knows? The prediction report suggest adding the following line to the labels-file:
~~~~~~~~~~~~~~~~
hint 8186C0 branch_always
~~~~~~~~~~~~~~~~
This tells the prediction heuristics that this branch is always taken so don't try to follow the fall-through case. With the hint in the labels file we get:
~~~~~~~~~~~~~~~~
label_8186B8:
    /* mi 81 0B00 8186B8 08          */ php
    /* mi 81 0B00 8186B9 E2 30       */ sep.b #$30
    /* mi 81 0B00 8186BB AE 31 04    */ ldx.w $0431
    /* mi 81 0B00 8186BE E0 10       */ cpx.b #$10
    /* mi 81 0B00 8186C0 D0 18       */ bne.b label_8186DA

.DB $A5,$87,$4A,$EB,$A5,$85,$4A,$C2,$30,$85,$02,$8A,$0A,$85,$0E,$64,$0C,$A6,$0C,$BD,$16,$05,$D0,$02

label_8186DA:
    /* mi 81 0B00 8186DA 28          */ plp
    /* mi 81 0B00 8186DB 60          */ rts
~~~~~~~~~~~~~~~~
We might never know what are contained in those bytes for now let's keep them as data. If we for some reason *know* that they are data we can also add a data annotation there. That way they will not be predicted as code either.

Some words about the heuristic
==============================
Prediction follow branches, long branches and jumps. For branches it also tries to fall-through case (except for the BRA-branch of course which never can fall through). Whenever a return of some form (RTI, RTS, RTL) is found prediction stops. Whenever it needs to determine the size of the instruction that needs the instruction sizes and it is unsure about instruction sizes it gives up.

Closing Word
============
Code prediction is very powerful since it clean up a lot of small things that catch your eye and burden you with thought. The prediction report is very useful to catch the places where it fails. Sometimes it is hard to see where the incorrect prediction started and it can take some time and practice to get it right! In the [next post](tutorial-auto) we will look at the auto-annotation feature that is really handy if you hate doing boring work! The labels file for this tutorial post can be found [here](code/tutorial-predict.labels). Remember that multiple labels file can be loaded at the same time using multiple *-labels* statements.
