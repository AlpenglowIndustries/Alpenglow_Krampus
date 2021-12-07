/*
Software that controls the Krampus Badge
Makes the red LEDs behind the eyes and tongue flicker
Mode switch controls flickering vs solidly on

Pins:
1 - PB0 - Eyes LEDs (thru FET)
2 - GND
3 - PB1 - Tongue LEDs (thru FET)
4 - PB2 - Mode switch
5 - Vcc
6 - Reset

Programming the ATTiny4:
** NOTE!  The ATTiny4/5/9/10 use the TPI programming interface, which is different from
other ATTinys like the ATTiny85 which uses PDI, and also different from the ISP interface
used by the ATMega series and most Arduinos.  You will need a USBasp programmer or an
original genuine Atmel AVRISP mk II.  AVRdude does not support TPI on other programmers
(sorry Atmel ICE users, you're currently SOL).**

- Arduino IDE with ATTiny10 Core by technoblogy:
  https://github.com/technoblogy/attiny10core
  http://www.technoblogy.com/show?1YQY (excellent programming instructions and blink code)
- Add http://www.technoblogy.com/package_technoblogy_index.json to Boards Urls in preferences
- Install ATTiny10 core using Boards Manager
- Set "Board" to ATTiny10/9/5/4
- Set "Chip" to ATTiny4
- Power must be 5V
- A switch has been provided on the RBG_RGB board to isolate pins during programming.
  Make sure it has been switched from "RUN" mode to "PRG".
  If you are not using our dev board, note that Pin 1, TPIDATA, cannot be driving anything of
  consequence downstream during programming.  No direct driving LEDs with this pin.
- Must "Upload using programmer" - no room for a bootloader!  No serial port!
- Programmers supported:
  Genuine original Atmel AVRISP mkII using Libusb-win32 driver.  Use Zadig to change driver if
    you plug it in and get a USB device not found error (using the default Atmel driver).  Note
    that many of the new knock-offs of the AVRISP mk II use USBtiny firmware which does NOT
    support TPI at this time.  Note the AVRISP mkII uses MISO for TPIDATA.
  USBasp - firmware must be updated to 2011-05-28 version to support TPI.  Download from
    https://www.fischl.de/usbasp/   Firmware can be updated via Arduino IDE, choose the hex
    file for the processor on your USBasp (different people use different ones).  Use an Arduino
    as ISP or a different programmer (ex: USBtiny).  After the update, should program TPI.  You
    may need to update the driver to libusb-win32 for Windows 10.  Use Zadig.
    Note the USBasp uses MOSI for TPIDATA.

Defaults on reset:
- Clock = 1 MHz (8 MHz internal oscillator / 8)
- Timer Module enabled, normal port operation, OCR0A/B disabled, no clock source (timer stopped),
  clock = system clock no prescaling

*/

//#include <util/atomic.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>

#define MAXEYES  255
#define MAXTONGUE 150
#define MINBRITE  5   // minimum PWM resolution is technically 3, but it hangs if lower

volatile uint32_t counter;
uint8_t times[] = {17, 50, 33};
uint8_t repeats[] = {6, 2, 4};
uint8_t loops = 0;
uint8_t mode = 0;

// PB0 = eyes
// PB1 = tongue
// PB2 = mode

// Variable count
// 4 counter
// 1 mode
// 1 wait
// 1 change
// 1 eyeBright
// 1 loops
// 1 loop counter
// ===============
// 10 bytes RAM max

void delay(uint16_t time) {  // brute force semi-accurate delay routine that doesn't use a timer
  counter = 46 * time;
  do counter--; while (counter != 0);
}

int main(void) {
                                                      // all pins set to inputs by default
  DDRB = (1 << DDB0 | 1 << DDB1);                     // PB0, PB1 set to outputs (eyes, tongue)
  PORTB = (1 << PORTB2);                              // Pullup resistor enabled on input pin PB2

  // TCCR0A = (1 << COM0A1 | 1 << COM0B1 | 1 << WGM00);  // Phase-correct PWM 8-bit mode, A and B
  // TCCR0B = (1 << CS01);                               // clk/8, timer started, 125 kHz timer clock
  //                                                     // and 245 Hz PWM frequency

  /*
  The General Gist:
  Two LEDs behind Krampus' eyes are attached to PB0
  Four LEDs behind Krampus' tongue are attached to PB1
  The mode switch is attached to PB2 using an internal pullup.
    Mode = 0 is pulsing
    Mode = 1 is solid on
  All LEDs are driven through transistors to reduce the load on the processor.
    The LEDs are hardwired to V+.  Switching GND toggles the LEDs on/off.  Therefore,
    the transistor-driven LEDs have "normal" logic (high means LED is ON).
  */

  //uint8_t wait = 20;
  //int8_t change = 1;
  //uint8_t eyeBright = MINBRITE + 1;



  while (1) {

    mode = PINB & (1 << PINB2);   // reads input on PB2
    while (mode) {
      // flickers
      uint8_t i;
      for (i = 0; i < repeats[loops]; i++) {
        PINB |= (1 << PB0) | (1 << PB1);
        delay(times[loops]*10);
      }
      loops = (loops + 1) % 5;
      mode = PINB & (1 << PINB2);
    }

    PORTB &= ~( (1 << PB0) | (1<< PB1)); // turns outputs off



    // // cycles through pulsing up and down on eyes & tongue, speed determined by wait
    // if (eyeBright == MINBRITE || eyeBright == MAXEYES) {
    //   change = -change;
    // }

    // OCR0A = eyeBright;
    // OCR0B = eyeBright;
    // eyeBright += change;
    // delay(wait);

  }   // end while(1)
}     // end main
