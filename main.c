#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

/*
UART (universal Asyncrnous Receiver-Transmitter) is a dedicated piece of silicon inside the controller
that takes a whole byte eg "H" splits it into bits and shoots them out of a pin at a very specific
speed

We use gpio0 for transmit and gpio0 for receive
*/

// Get the memory address of bank to set gpio mode
#define IO_BANK0_BASE 0x40028000

// Get the memory offset of the GPIO0 control register
#define GP0_CTRL_OFFSET 0x004

// Same for GPIO1 
#define GP1_CTRL_OFFSET 0x00c

// Get the base memory address for the UART0 settings
#define UART0_BASE 0x40070000

/* UART communication requires both sides (PC + microcontroller) to communicate on the same agreed
speed, called the "Baud Rate" (Bits per second)

By default pico 2w is running on its internal ring oscillator at around 12mhz which is not stable
so we will run the Baud at 9600

From UART memory section we need to get the UARTIBRD (integer Baud Rate Divizor) and 
UARTFBRD (float Baud Rate Divizor) registers

The math: Divider = UART_Clock / (16 * Baud_Rate) Assuming clock is roughly 12,000,000 Hz =>

12000000 / (16 * 9600) = 78,125 => int = 78 and float = 125

*/

#define UARTIBRD_OFFSET 0x024
#define UARTFBRD_OFFSET 0x028

/* Serial comm has a standard format called 8N1 - 8 data bits, 0 parity bits, 1 stop bit we need
to tell UART to use this format.

We do this by using the UARTLCR_H(Line control register) register and setting its WLEN(word len) to
11 which means 8 bits and set FEN to 1 which enables the hardware FIFOs to prevent traffic jams
*/

#define UARTLCR_H_OFFSET 0x02c

/* We need to actually enable UART enginge on the board we do this using the UARTCR (Control Register)
We need to enable the UART (UARTEN), the transmit (TXE) and the receive (RXE)
*/

#define UARTCR_OFFSET 0x030

/*To send a letter we just write it to the UARTDR (Data Register). However if we write letters
faster than the hardware can shoot them out we will overwrite them so we need to check the
UARTFR (flag register) at the TXFF (Transmit FIFO full) bit
*/

#define UARTDR_OFFSET 0x000
#define UARTFR_OFFSET 0x018

// Gets pads to remove isolation from GP0 and GP1
#define PADS_BANK0_BASE 0x40038000
#define PADS_GPIO0_OFFSET 0x04
#define PADS_GPIO1_OFFSET 0x08

/*Modern microcontrollers like pico 2w dont turn on all peripherals at boot in otder to save power
by default complex peripherals like UART, SPI, I2C and ADC are kept in a state of permanent reset

We need to wakeup the peripherals pe want to use in this case UART, we do this at the RESET reg

To know when the component has woken up we need to look at the DONE register
*/

#define RESETS_BASE 0x40020000
#define RESET 0x0
#define RESET_DONE_OFFSET 0x8


//Function to wait for UARTFR to be empty and then write a letter
void write_char(char c){
    volatile uint32_t * UARTFR = (volatile uint32_t *)(UART0_BASE + UARTFR_OFFSET);
    // TXFF is the 5th bit, we wait until it is 0
    while((*UARTFR >> 5) & 1){}

    // Send the char
    *(volatile uint32_t *)(UART0_BASE + UARTDR_OFFSET) = c;
}

int main(){
    //Turn UART0 on (stop it from endless reset)
    *(volatile uint32_t *)(RESETS_BASE + RESET) &= ~(1 << 26);
    //Wait for the wake up to happen
    volatile uint32_t * reset_done = (volatile uint32_t *)(RESETS_BASE + RESET_DONE_OFFSET);
    while(!((*reset_done >> 26) & 1)){}

    //Open the latches for GP0 and GP1
    *(volatile uint32_t*)(PADS_BANK0_BASE + PADS_GPIO0_OFFSET) &= ~(1 << 8);
    *(volatile uint32_t*)(PADS_BANK0_BASE + PADS_GPIO1_OFFSET) &= ~(1 << 8);

    //Set GP0 to UART tx
    *(volatile uint32_t *)(IO_BANK0_BASE + GP0_CTRL_OFFSET) = 0x02;
    //Set GP1 to UART receive
    *(volatile uint32_t *)(IO_BANK0_BASE + GP1_CTRL_OFFSET) = 0x02;
    //Set the Baud Int Rate to 78 
    *(volatile uint32_t *)(UART0_BASE + UARTIBRD_OFFSET) = 78;
    //Set the Baud Float Rate to 8 Fraction = int(fract_part(125) * 64 + 0.5) = 8
    *(volatile uint32_t *)(UART0_BASE + UARTFBRD_OFFSET) = 8;
    //Set the word len to 8 bits, enable FIFO
    *(volatile uint32_t *)(UART0_BASE + UARTLCR_H_OFFSET) = (3 << 5) | (1 << 4);
    //Enable the UART engine on the board (UARTEN, TXE, RXE) 
    *(volatile uint32_t *)(UART0_BASE + UARTCR_OFFSET) = 1 | (1 << 8) | (1 << 9);
    
    
    while(1){
        write_char('a');
    }

    return 0;
}