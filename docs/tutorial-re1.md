---
title: Tutorial 6: Back to reverse engineering!
layout: default
---
This post continues where the [first post](tutorial-intro) left off. Now we have gained a lot of knowledge about how snestistics works and we have access to assembly listing for out game. Searching for the PC address where our tile map breakpoint hit in the disassembly, and using a bit of 65816 knowledge it seems likely that this range is a self contained function that takes care of writing tile map data to VRAM:
```
Auto0005:
    /* m* 80      80820C E2 20       */ sep.b #$20
    /* m* 80      80820E AD 80 02    */ lda.w $0280                     ; 7E0280 [DB=80]
    /* m* 80      808211 10 01       */ bpl.b _Auto0005_808214

_Auto0005_808213:
    /* mI 80      808213 60          */ rts

_Auto0005_808214:
    /* m* 80      808214 C2 10       */ rep.b #$10
    /* mI 80      808216 AE 8E 02    */ ldx.w $028E                     ; 7E028E [DB=80]
    /* mI 80      808219 F0 F8       */ beq.b _Auto0005_808213
    /* mI 80      80821B A2 00 00    */ ldx.w #$0000

_Auto0005_80821E:
    /* *I 80      80821E E2 20       */ sep.b #$20
    /* mI 80      808220 A9 80       */ lda.b #$80
    /* mI 80      808222 BC 00 01    */ ldy.w $0100,x                   ; 7E0100 [DB=80 X=0, 6, 8, A, C, 10, 12, 14, <18, 1A, ..., 22>, 24, 26, 28, <2C, 2E, ..., 36>, <38, 3C, ..., 48> and 20 more]
    /* mI 80      808225 F0 24       */ beq.b _Auto0005_80824B
    /* mI 80      808227 10 01       */ bpl.b _Auto0005_80822A
    /*pmI 80 0000 808229 1A          */ inc A

_Auto0005_80822A:
    /* mI 80      80822A 8D 15 21    */ sta.w REG_VMAIN                 ; Video Port Control Register - [DB=80]
    /* mI 80      80822D C2 20       */ rep.b #$20
    /* MI 80      80822F 98          */ tya
    /* MI 80      808230 29 FF 00    */ and.w #$00FF
    /* MI 80      808233 A8          */ tay
    /* MI 80      808234 BD 02 01    */ lda.w $0102,x                   ; 7E0102 [DB=80 X=0, 6, 8, A, C, 10, 12, 14, 18, 1C, 1E, 20, 24, 26, 28, 2C, 2E, 30, <34, 38, ..., 48> and 19 more]
    /* MI 80      808237 8D 16 21    */ sta.w REG_VMADDL                ; VRAM Address Registers (Low) - [DB=80]

_Auto0005_80823A:
    /* MI 80      80823A BD 04 01    */ lda.w $0104,x                   ; 7E0104 [DB=80 X=<0, 2, ..., 134>]
    /* MI 80      80823D 8D 18 21    */ sta.w REG_VMDATAL               ; VRAM Data Write Registers (Low) - [DB=80]
    /* MI 80      808240 E8          */ inx
    /* MI 80      808241 E8          */ inx
    /* MI 80      808242 88          */ dey
    /* MI 80      808243 D0 F5       */ bne.b _Auto0005_80823A
    /* MI 80      808245 E8          */ inx
    /* MI 80      808246 E8          */ inx
    /* MI 80      808247 E8          */ inx
    /* MI 80      808248 E8          */ inx
    /* MI 80      808249 80 D3       */ bra.b _Auto0005_80821E

_Auto0005_80824B:
    /* mI 80      80824B C2 20       */ rep.b #$20
    /* MI 80      80824D 9C 8E 02    */ stz.w $028E                     ; 7E028E [DB=80]
    /* MI 80      808250 60          */ rts
```
> Note: Your disassembly may look quite different since snestistics is very much in active development! The code quoted above comes from a disassembly using "auto annotate", but with no labels file supplied.

Since this function writes to VRAM, it must be called during the vertical blanking interval (or when the screen is forced blank). Searching for where `Auto0005` is called reveals that it is only used in `Auto0004` just above it. `Auto004` is in turn part of a small set of functions called in the NMI handler. This is also where the previously inspected joypad readout function is found. 

Without inspecting this code too closely, we can observe a couple of things:
- `80820E`: An 8-bit value is loaded from `$0280`. If the most significant bit (minus sign) is set the function will exit immediately. 
- `808216`: A 16-bit value is loaded from `$028E`. If zero the function will exit.
- Then a loop indexing data at `$0100` runs, with an exit condition at `808225`.

Based on these observations we now know that code running elsewhere, and probably not in vertical blanking, will add entries to an array at `$0100` and setting some "data available"-flags at `$0280` and `$028E`. The entries will then be written to VRAM the next time the NMI handler runs.

All this follows typical SNES coding conventions; at the start of the vertical blanking (vblank) interval an NMI interrupt is fired, and in its interrupt handler all PPU RAM (VRAM as well as object and color RAM) is updated. A while into vblank the CPU has also put new data in the joypad registers, so it's common practice to read out that data after some other task inside the NMI handler is finished.

The next step is to put breakpoints at `$0100`, `$0280` and `$028E` to see how the array of VRAM writes get populated. But first let's add a function range and some labels to our labels-file to make this function more readable:
```
; Copy data to VRAM
;   Called during NMI
;   Parameters at $0100
;   Note: Slow, probably only used for small bits of map data
# Copy data to VRAM (called during NMI)
function 80820C 808250 NMI_vram_memcpy
label 808213 exit
label 80821E loop
label 80823A inner
label 80824B done
```

Now the same section of the disassembly looks like this:
```
; Copy data to VRAM
;   Called during NMI
;   Parameters at $0100
;   Note: Slow, probably only used for small bits of map data
NMI_vram_memcpy:
    /* m* 80      80820C E2 20       */ sep.b #$20
    /* m* 80      80820E AD 80 02    */ lda.w $0280                     ; 7E0280 [DB=80]
    /* m* 80      808211 10 01       */ bpl.b _NMI_vram_memcpy_808214

_NMI_vram_memcpy_exit:
    /* mI 80      808213 60          */ rts

_NMI_vram_memcpy_808214:
    /* m* 80      808214 C2 10       */ rep.b #$10
    /* mI 80      808216 AE 8E 02    */ ldx.w $028E                     ; 7E028E [DB=80]
    /* mI 80      808219 F0 F8       */ beq.b _NMI_vram_memcpy_exit
    /* mI 80      80821B A2 00 00    */ ldx.w #$0000

_NMI_vram_memcpy_loop:
    /* *I 80      80821E E2 20       */ sep.b #$20
    /* mI 80      808220 A9 80       */ lda.b #$80
    /* mI 80      808222 BC 00 01    */ ldy.w $0100,x                   ; 7E0100 [DB=80 X=0, 6, 8, A, C, 10, 12, 14, <18, 1A, ..., 22>, 24, 26, 28, <2C, 2E, ..., 36>, <38, 3C, ..., 48> and 20 more]
    /* mI 80      808225 F0 24       */ beq.b _NMI_vram_memcpy_done
    /* mI 80      808227 10 01       */ bpl.b _NMI_vram_memcpy_80822A
    /*pmI 80 0000 808229 1A          */ inc A

_NMI_vram_memcpy_80822A:
    /* mI 80      80822A 8D 15 21    */ sta.w REG_VMAIN                 ; Video Port Control Register - [DB=80]
    /* mI 80      80822D C2 20       */ rep.b #$20
    /* MI 80      80822F 98          */ tya
    /* MI 80      808230 29 FF 00    */ and.w #$00FF
    /* MI 80      808233 A8          */ tay
    /* MI 80      808234 BD 02 01    */ lda.w $0102,x                   ; 7E0102 [DB=80 X=0, 6, 8, A, C, 10, 12, 14, 18, 1C, 1E, 20, 24, 26, 28, 2C, 2E, 30, <34, 38, ..., 48> and 19 more]
    /* MI 80      808237 8D 16 21    */ sta.w REG_VMADDL                ; VRAM Address Registers (Low) - [DB=80]

_NMI_vram_memcpy_inner:
    /* MI 80      80823A BD 04 01    */ lda.w $0104,x                   ; 7E0104 [DB=80 X=<0, 2, ..., 134>]
    /* MI 80      80823D 8D 18 21    */ sta.w REG_VMDATAL               ; VRAM Data Write Registers (Low) - [DB=80]
    /* MI 80      808240 E8          */ inx
    /* MI 80      808241 E8          */ inx
    /* MI 80      808242 88          */ dey
    /* MI 80      808243 D0 F5       */ bne.b _NMI_vram_memcpy_inner
    /* MI 80      808245 E8          */ inx
    /* MI 80      808246 E8          */ inx
    /* MI 80      808247 E8          */ inx
    /* MI 80      808248 E8          */ inx
    /* MI 80      808249 80 D3       */ bra.b _NMI_vram_memcpy_loop

_NMI_vram_memcpy_done:
    /* mI 80      80824B C2 20       */ rep.b #$20
    /* MI 80      80824D 9C 8E 02    */ stz.w $028E                     ; 7E028E [DB=80]
    /* MI 80      808250 60          */ rts
```

With a bit of experience it's quite easy to determine the structure of the data at `$0100`, but we'll leave that excerise to the reader for now!

Next steps
==========
We've now discovered the function responsible for copying characters to VRAM from an array at `$0100`. Elsewhere in the program data has to be read from ROM, possible undergo some kind of transformation, and get written to that array. Putting a write breakpoint at CPU memory location `$0100` should give some answers!
