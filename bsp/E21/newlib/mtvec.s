/* Copyright(C) 2020 Hex Five Security, Inc. - All Rights Reserved */

        .align 2

_mtvec:

irq0:   .word trp_isr
irq1:   .word 1f
irq2:   .word 1f
irq3:   .word msi_isr
irq4:   .word 1f
irq5:   .word 1f
irq6:   .word 1f
irq7:   .word tmr_isr
irq8:   .word 1f
irq9:   .word 1f
irq10:  .word 1f
irq11:  .word 1f
irq12:  .word 1f
irq13:  .word 1f
irq14:  .word 1f
irq15:  .word 1f
irq16:  .word 1f
irq17:  .word 1f
irq18:  .word 1f
irq19:  .word dma_isr
irq20:  .word btn0_isr
irq21:  .word btn1_isr
irq22:  .word btn2_isr
irq23:  .word 1f
irq24:  .word 1f
irq25:  .word 1f
irq26:  .word 1f
irq27:  .word 1f
irq28:  .word 1f
irq29:  .word 1f
irq30:  .word 1f
irq31:  .word 1f

.macro FILL from, to
.word 1f
.if \to-\from
FILL "(\from+1)",\to
.endif
.endm

FILL 32, 99
FILL 100, 158

irq159: .word uart_isr

        .weak trp_isr, msi_isr, tmr_isr, dma_isr, uart_isr, btn0_isr, btn1_isr, btn2_isr

trp_isr:
msi_isr:
tmr_isr:
dma_isr:
uart_isr:
btn0_isr:
btn1_isr:
btn2_isr:
1:      j .
