/* Copyright(C) 2020 Hex Five Security, Inc. - All Rights Reserved */

        .align 2

_mtvec:

        .option push
        .option norvc

irq0:   j trp_isr
irq1:   j .
irq2:   j .
irq3:   j msi_isr
irq4:   j .
irq5:   j .
irq6:   j .
irq7:   j tmr_isr
irq8:   j .
irq9:   j .
irq10:  j .
irq11:  j plic_isr
irq12:  j .
irq13:  j .
irq14:  j .
irq15:  j .
irq16:  j .
irq17:  j .
irq18:  j .
irq19:  j .
irq20:  j .
irq21:  j .
irq22:  j .
irq23:  j .
irq24:  j .
irq25:  j .
irq26:  j .
irq27:  j .
irq28:  j .
irq29:  j .
irq30:  j .
irq31:  j .

        .option pop

        .weak trp_isr, msi_isr, tmr_isr, plic_isr

trp_isr:
msi_isr:
tmr_isr:
plic_isr:
        j .
