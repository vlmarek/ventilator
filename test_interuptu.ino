
#include <avr/sleep.h>    // Sleep Modes
#include <avr/power.h>    // Power management
#include <avr/wdt.h>      // Watchdog timer

const byte LED_TIME = 9;    // yellow
const byte LED_BUTTON = 13; // red
const byte SWITCH = 2;

volatile int button_pressed;
volatile int watchdog_happened;

ISR (WDT_vect)
{
  if (!watchdog_happened) {
    watchdog_happened = 1;
  }
}

void blink ()
{
  if (!button_pressed) {
    button_pressed = 1;
  }
}

void resetWatchdog ()
{
  // WDP3 WDP2 WDP1 WDP0 Typical Time-out at VCC = 5.0V
  // 0 0 0 0 16ms
  // 0 0 0 1 32ms
  // 0 0 1 0 64ms
  // 0 0 1 1 0.125 s
  // 0 1 0 0 0.25 s
  // 0 1 0 1 0.5 s
  // 0 1 1 0 1.0 s
  // 0 1 1 1 2.0 s
  // 1 0 0 0 4.0 s
  // 1 0 0 1 8.0 s

  // clear various "reset" flags
  MCUSR = 0;
  // allow changes, disable reset, clear existing interrupt
  WDTCSR = bit (WDCE) | bit (WDE) | bit (WDIF);
  // set interrupt mode and an interval (WDE must be changed from 1 to 0 here)
  WDTCSR = bit (WDIE) | bit (WDP2) | bit (WDP0) ; // 0.5s
//  WDTCSR = bit (WDIE) | bit (WDP3) | bit (WDP0) ; // 8s

  // re/start the watchdog
  wdt_reset();
}  // end of resetWatchdog


void setup ()
{
  resetWatchdog ();  // do this first in case WDT fires

  pinMode (LED_TIME, OUTPUT);
  pinMode (LED_BUTTON, OUTPUT);
  pinMode (SWITCH, INPUT_PULLUP);
  digitalWrite (SWITCH, HIGH);  // internal pull-up
  
  set_sleep_mode (SLEEP_MODE_PWR_DOWN);
  ADCSRA = 0;            // turn off ADC (is this needed if we have power_all_disalbe() ?)
  sleep_bod_disable();   // disable brown out detection
  power_all_disable ();  // power off ADC, Timer 0 and 1, serial interface

  button_pressed = 0;
  watchdog_happened = 0;

  attachInterrupt(digitalPinToInterrupt(SWITCH), blink, CHANGE);
}  // end of setup

void loop ()
{
  if (button_pressed) {
    digitalWrite (LED_BUTTON, !digitalRead(LED_BUTTON));
    unsigned long debounce = millis();
    unsigned long debounce_start = debounce;
    power_all_enable();
    while ( millis() - debounce < 20 ) {
      if (!digitalRead(SWITCH)) {
        debounce = millis();
      }
    }
    power_all_disable();
    button_pressed = 0;
  }
  if (watchdog_happened) {
    watchdog_happened = 0;
    digitalWrite (LED_TIME, !digitalRead(LED_TIME));
  }

  noInterrupts ();       // timed sequence coming up
  sleep_enable ();       // ready to sleep
  interrupts ();         // interrupts are required now
  sleep_cpu ();          // sleep
  sleep_disable ();      // precaution
}  // end of goToSleep
