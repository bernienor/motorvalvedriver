/**
 * @brief Driver for exhaustvalve for motor. HVL project spring 2018
**/


#define encoderPinA 2
#define encoderPinB 3
#define outputPin 13 // LED pin for test

volatile unsigned int pulsecounter = 0;


/**
 * @brief Set up
 * Sets up two interrupt inputs for the A and B output of the encoder.
 * Sets up one interrupt input (comparator input) for the one puls pr revolution signal.
 * Sets up the serial port for receiving set values for the output.
**/
void setup() {
  pinMode(encoderPinA, INPUT);
  pinMode(encoderPinB, INPUT);

 /// Setup doEncoder to service both A and B signal interrupt on change.
  attachInterrupt(0, doEncoder, CHANGE);
  attachInterrupt(1, doEncoder, CHANGE);

  Serial.begin (115200);
}


/**
 * @todo Add Debugoutput on serial port
 * @todo Add serial port handler
 *
**/
void loop() {
  



}




/**
 * @brief Interrupt routine for the A and B interrupt lines. 
 * Both interrupt sources are handled by the same inerrupt service routine. 
**/
void doEncoder() {
  pulsecounter++;
  if(pulsecounter == onvalue){
    digitalWrite(outputPin, HIGH);
  } else if((pulsecounter == offvalue){
    digitalWrite(outputPin, LOW);
  }
}
