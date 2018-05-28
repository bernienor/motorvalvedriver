/**
 * @brief timer1 module based on code from here: http://code.google.com/p/arduino-timerone/
 * 
 */

#ifndef TIMERONE_cpp
#define TIMERONE_cpp

#include "TimerOne.h"

TimerOne Timer1;              // preinstatiate

ISR(TIMER1_OVF_vect)          // interrupt service routine that wraps a user defined function supplied by attachInterrupt
{
  Timer1.isrCallback();
}


void TimerOne::initialize(long microseconds)
{
  TCCR1A = 0;                 // clear control register A 
  TCCR1B = _BV(WGM13);        // set mode 8: phase and frequency correct pwm, stop the timer
  setPeriod(microseconds);
}

void TimerOne::setPeriod(long microseconds)    // AR modified for atomic access
{
  
  long cycles = (F_CPU / 2000000) * microseconds;                                // the counter runs backwards after TOP, interrupt is at BOTTOM so divide microseconds by 2
  if(cycles < RESOLUTION)              clockSelectBits = _BV(CS10);              // no prescale, full xtal
  else if((cycles >>= 3) < RESOLUTION) clockSelectBits = _BV(CS11);              // prescale by /8
  else if((cycles >>= 3) < RESOLUTION) clockSelectBits = _BV(CS11) | _BV(CS10);  // prescale by /64
  else if((cycles >>= 2) < RESOLUTION) clockSelectBits = _BV(CS12);              // prescale by /256
  else if((cycles >>= 2) < RESOLUTION) clockSelectBits = _BV(CS12) | _BV(CS10);  // prescale by /1024
  else        cycles = RESOLUTION - 1, clockSelectBits = _BV(CS12) | _BV(CS10);  // request was out of bounds, set as maximum
  
  oldSREG = SREG;       
  cli();              // Disable interrupts for 16 bit register access
  ICR1 = pwmPeriod = cycles;                                          // ICR1 is TOP in p & f correct pwm mode
  SREG = oldSREG;
  
  TCCR1B &= ~(_BV(CS10) | _BV(CS11) | _BV(CS12));
  TCCR1B |= clockSelectBits;                                          // reset clock select register, and starts the clock
}

void TimerOne::resume()        // AR suggested
{ 
  TCCR1B |= clockSelectBits;
}

void TimerOne::stop()
{
  TCCR1B &= ~(_BV(CS10) | _BV(CS11) | _BV(CS12));          // clears all clock selects bits
}


void TimerOne::start()  // AR addition, renamed by Lex to reflect it's actual role
{
  unsigned int tcnt1;
  
  TIMSK1 &= ~_BV(TOIE1);        // AR added 
  GTCCR |= _BV(PSRSYNC);      // AR added - reset prescaler (NB: shared with all 16 bit timers);

  oldSREG = SREG;       // AR - save status register
  cli();            // AR - Disable interrupts
  TCNT1 = 0;                  
  SREG = oldSREG;             // AR - Restore status register
  resume();
  do {  // Nothing -- wait until timer moved on from zero - otherwise get a phantom interrupt
  oldSREG = SREG;
  cli();
  tcnt1 = TCNT1;
  SREG = oldSREG;
  } while (tcnt1==0); 

  
void TimerOne::attachInterrupt(void (*isr)(), long microseconds)
{
  if(microseconds > 0) setPeriod(microseconds);
  isrCallback = isr;                                       // register the user's callback with the real ISR
  TIMSK1 = _BV(TOIE1);                                     // sets the timer overflow interrupt enable bit
 // might be running with interrupts disabled (eg inside an ISR), so don't touch the global state
//  sei();
  resume();                       
}


void TimerOne::detachInterrupt()
{
  TIMSK1 &= ~_BV(TOIE1);                                   // clears the timer overflow interrupt enable bit 
                              // timer continues to count without calling the isr
}

