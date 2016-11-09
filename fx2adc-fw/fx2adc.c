/*
 * This is some relatively quick and dirty firmware to initialize the FX2's
 * data bus in 16 bit asynchronous slave fifo mode.  In this mode, data is 
 * presented on FD[15:0] and the active low SLWR signal is used to clock in
 * data asynchronously.  Endpoint 6 is activated in bulk mode, Auto-In enabled, 
 * with the data packet size set to 512 bytes.  This means that once 512 bytes 
 * (256 words) have been clocked in, the data will be automatically transfered 
 * to the FX2's USB engine. See FX2 Technical Reference Manual Ch. 9 for more
 * information/further configuration options.
 * NOTE: According to the FX2 datasheet the SLWR pin should be held low for a 
 * minimum of 50ns and high for a minimum of 70ns.  This limits data transfer in 
 * asynchronous mode to ~12MB/s.  This limitation does not apply in 
 * synchronous FIFO mode (96MB/s).
 */

#define ALLOCATE_EXTERN
#include "fx2regs.h"

//NOTE: This delay value is only valid for CPU CLK and IFCLK = 48Mhz.
#define	SYNCDELAY	__asm nop; nop; nop; __endasm
#define	NOP		__asm nop; __endasm
#define	CONVDELAY __asm nop; nop; nop; nop; nop; __endasm

//Initializes the hardware in to the above described mode
void init()
{
  CPUCS = bmCLKSPD1; // CPU runs @ 48 MHz
  
  SYNCDELAY;
  REVCTL = 0x03; //The TRM says to set this or ELSE

  //Clear and reset all FIFOs
  SYNCDELAY;
  FIFORESET = 0x80;
  SYNCDELAY;
  FIFORESET = 0x02;
  SYNCDELAY;
  FIFORESET = 0x04;
  SYNCDELAY;
  FIFORESET = 0x06;
  SYNCDELAY;
  FIFORESET = 0x08;
  SYNCDELAY;
  FIFORESET = 0x00;
  SYNCDELAY;

  IFCONFIG=0xCB;

  SYNCDELAY;
  //EP6CFG = 0xE2; //EP6 enabled, direction: IN, type: BULK, Buffer size: 512, Double Buffered
  EP6CFG = 0xE0; //EP6 enabled, direction: IN, type: BULK, Buffer size: 512, Quad Buffered

  SYNCDELAY;
  /*
  EP6FIFOCFG = 0x0D; //AUTO-IN enabled, Zero-length packets enabled, 
                     //Worldwide=1 (16 bit bus with)
  */
  EP6FIFOCFG = 0x08; //AUTO-IN enabled, Zero-length packets disabled, 
                     //Worldwide=0 (8 bit bus with)

  SYNCDELAY;
  
  PORTACFG=0x00;
  FIFOPINPOLAR=0x00; //default polarities: SLWR active low

  SYNCDELAY;
  //This determines how much data is accumulated in the FIFOs before a USB packet
  //is committed.  I've had no luck with more than 512, up to 1024 should be supported.
  //Perhaps this limitation is a libusb issue?
  EP6AUTOINLENH = 0x02; //MSB
  SYNCDELAY;
  EP6AUTOINLENL = 0x00; //LSB
  
  SYNCDELAY; //for good measure  
}


void main(void)
{
  init();
  
  while (1) SYNCDELAY;
}
