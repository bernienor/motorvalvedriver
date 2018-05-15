/**
 * @brief Driver for exhaustvalve for motor. HVL project spring 2018
 * 
 * Set up for test: 
 * Expect 1440 counts pr revolution.
 * onValue  = 360
 * offValue = 720
 * Prints the counted number of pulses pr revolution to the serial port
 * 
**/


#define encoderPinA 2
#define encoderPinB 3
#define outputPin 13 // LED pin for test

volatile unsigned int onValue = 30000; // Very high values, effectively disabled by default
volatile unsigned int offValue = 30001;
volatile unsigned int pulsecounter = 0;
volatile unsigned int lastcount = 0;


/**
 * @brief Analouge Comparator service routine
 * Resets the interrupt counter for inputs A and B.
**/
ISR (ANALOG_COMP_vect) {
  cli();
  lastcount = pulsecounter;
  pulsecounter = 0;
  sei();
}

/**
 * @brief Interrupt routine for the A and B interrupt lines. 
 * Both interrupt sources are handled by the same inerrupt service routine. 
**/
void doEncoder() {
  pulsecounter++;
   if(pulsecounter == onValue){
    digitalWrite(outputPin, HIGH);
  } else if(pulsecounter == offValue){
    digitalWrite(outputPin, LOW);
  }
}

/**
 * @brief Set up
 * Sets up two interrupt inputs for the A and B output of the encoder.
 * Sets up one interrupt input (comparator input) for the one puls pr revolution signal.
 * Sets up the serial port for receiving set values for the output.
**/
void setup() {
  pinMode(encoderPinA, INPUT);
  pinMode(encoderPinB, INPUT);

 /// Set up doEncoder to service both A and B signal interrupt on change.
  attachInterrupt(0, doEncoder, CHANGE);
  attachInterrupt(1, doEncoder, CHANGE);

/// Set up analouge comparator interrupt
  ADCSRB = ADCSRB & ! bit(ACME);   // disable Analog Comparator Multiplexer Enable
  ACSR =  bit(ACI)                 // clear Analog Comparator Interrupt Flag
        | bit(ACIE)                // set Analog Comparator Interrupt Enable
        | bit(ACIS1) | bit(ACIS0)  // select rising edge: ACIS1/ACIS0 Analog Comparator Interrupt Mode Select
        ;

  Serial.begin(115200);
}

/**
 * @todo Add Debugoutput on serial port
 *
**/
void loop() {
  
  if(lastcount){
    Serial.print("Cnt: ");
    Serial.println(lastcount, DEC);
    lastcount = 0;
  }


}





