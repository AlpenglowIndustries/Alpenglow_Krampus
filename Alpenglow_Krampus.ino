/*
Software that controls the Krampus Badge
Makes the red LEDs behind the eyes and tongue flicker
Mode switch controls flickering vs pulsing behavior

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
- Turn ON/OFF switch to OFF to isolate 3V battery from 5V programming voltage!  Better yet, 
  take the battery out while you're developing code.
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

#include <avr/io.h>
#include <stdint.h>

#define MAXBRITE  255
#define MINBRITE  5   // minimum PWM resolution is technically 3, but it hangs if lower
#define WAIT      10

// all variables declared as globals, because there's plenty of space in dynamic memory (RAM) for this one, 
//    while program memory (flash) is getting a bit crowded.
// compiler is smart enough to know consts go in program storage, also smart enough to put them there
//   even if not defined as consts.
volatile uint32_t counter;
const uint8_t times[] = {17, 50, 25, 17, 50};   // flicker time, divided by 5 (so can be an array of 8 bit numbers instead of 16)
const uint8_t repeats[] = {3, 2, 6, 2, 2};      // number of times each of the above flicker times repeats, adding up to an odd number gives better random appearance
const uint8_t length = 5;                       // length of the above arrays
uint8_t pos = 0;
uint8_t mode = 0;
int8_t change = 1;
uint8_t brightness = MINBRITE + 1;

void delay(uint16_t time) {  // brute force semi-accurate delay routine that doesn't use a timer
  counter = 46 * time;
  do counter--; while (counter != 0);
}

int main(void) {
                                                      // all pins set to inputs by default
  DDRB = (1 << DDB0 | 1 << DDB1);                     // PB0, PB1 set to outputs (eyes, tongue)    
  PUEB = (1 << PUEB2);                                // Pullup resistor enabled on input pin PB2
                                                      // - alternate of writing 1 to PORTB doesn't work even though datasheet says it should??
                                        
  /*
  The General Gist:
  PB0 = eyes
  PB1 = tongue
  PB2 = mode
  Two LEDs behind Krampus' eyes are attached to PB0
  Four LEDs behind Krampus' tongue are attached to PB1
  The mode switch is attached to PB2 using an internal pullup.
    Mode = 0 is pulsing
    Mode = 1 is flickering
  All LEDs are driven through transistors to reduce the load on the processor.
    The LEDs are hardwired to V+.  Switching GND toggles the LEDs on/off.  Therefore,
    the transistor-driven LEDs have "normal" logic (high means LED is ON).
  */

  while (1) {

    mode = PINB & (1 << PINB2);   // reads input on PB2

    // Flicker behavior
    // alternates quick flashing on eyes and tongue
    // - uses the shortcut PIN register writing to toggle outputs
    // - uses "times" array to determine on or off times
    // - uses "repeats" array to determine number of cycles at each time
    // - uses "length" to wrap at the length of above arrays
    if (mode) {
      // sets up variables and pins for flickering
      TCCR0A = 0;                 // PWM/timer operations stopped
      TCCR0B = 0;                 
      PORTB = 1;                  // turns tongue off and eyes on so they alternate
      pos = 0;                    // resets position

      // flickers
      while (mode) {
        uint8_t i;
        for (i = 0; i < repeats[pos]; i++) {      // cycles the # of times in repeats[] array
          PINB |= (1 << PINB0) | (1 << PINB1);    // shortcut for toggling pin output
          delay(5*times[pos]);                    // delays for # of milliseconds in times[] array *5
        }
        pos = (pos + 1) % length;                 // increases position, wraps at # of positions in arrays
        mode = PINB & (1 << PINB2);               // reads mode pin
      }
      PORTB = 0;                                  // eyes and tongue off
    }

    // Pulse behavior
    // cycles through pulsing up and down on eyes & tongue
    // - speed determined by wait #define
    // - min and max brightness determined by #defines
    // - when reaches a min or max, switches between adding or subtracting 1
    // - neat way of looping up and down without creating 2 loops

    // sets up variables and pins for pulsing
                                                        // PWM setup, for 125 kHz timer cloc and 245 Hz PWM frequency
    TCCR0A = (1 << COM0A1 | 1 << COM0B1 | 1 << WGM00);  // Phase-correct PWM 8-bit mode, A and B  
    TCCR0B = (1 << CS01);                               // clk/8, timer started
    brightness = MINBRITE + 1;
    change = 1;
    
    // pulses
    while (!mode) {
      if (brightness == MINBRITE || brightness == MAXBRITE) {  // checks for direction change
        change = -change;
      }

      OCR0A = brightness;           // eyes = brightness
      OCR0B = brightness;           // tongue = brightness
      brightness += change;         // preps brightness for next time
      delay(WAIT);  
      mode = PINB & (1 << PINB2);   // reads mode pin
    }

//    PORTB |= (1 << PORTB0) | (1<< PORTB1);            // turns eyes and tongue solid on for photo mode, comment out above pulse if used

  }   // end while(1)
}     // end main
