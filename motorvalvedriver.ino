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

#define DEBUG 1

#define encoderPinA 2
#define encoderPinB 3
#define compPinA 6
#define compPinB 7
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
  pinMode(compPinA, INPUT);
  pinMode(compPinB, INPUT);
  
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
  test_float();
}


/**
 * @todo Add Debugoutput on serial port
 *
**/
void loop() {
  if(Serial.available()){
    serport_handler(Serial.read());
  }
}


/**
 * @brief Receives data from serial port. And hendles these data into new config. 
 * The protocoll is semi-modbus. Just to get the CRC and to allow for further expansion later.
 * Will only accept update of register 40000-40003 in one write. That is Modbus preset multiple 
 * register command using RTU framing to modbus slave 1. Ref: http://modbus.org/docs/PI_MBUS_300.pdf
 * Deciffered message (hex):
 * 01 10 00 00 00 04 08 aa aa aa aa bb bb bb bb crc crc
 * Where aa aa aa aa is the four bytes of the float for On time and
 * bb bb bb bb bb is the four bytes of the float for Off time.
 * crc crc is the two bytes of the 16 bit CRC.
 * The package is always 17 bytes long.
 * 
 * To make sure the communication will get into sync again after receiving a noise byte only one byte is decarded at 
**/
void serport_handler(char d)
{
  static char inputbuffer[20];
  static int endindex=0;

  inputbuffer[endindex++] = d;
  if(endindex == 17){
    if(process_serialdata(inputbuffer) ==-1){
      Serial.write("Error!");      
      discard_byte(inputbuffer,endindex--);   
    }
    endindex = 0;
  }
  if(endindex >=20){
    Serial.write("Endindex overflow!"); 
  }
}


/** @brief simple processing of "semi modbus" message
 *  @returns -1 on error, 0 on success
**/
int process_serialdata(char* buf)
{
  unsigned int crc;
  float on,off;

  Serial.print(".");
  
  if(buf[0] != 0x01)
    return(-1);
    
  if(buf[1] != 0x10)
    return(-1);
    
  for(int i = 2; i<5; i++)
    if(buf[i] != 0x00)
      return(-1);

  if(buf[5] != 0x04)
    return(-1);

  if(buf[6] != 0x08)
    return(-1);

  crc = ModRTU_CRC(buf, 17); 
  if(crc != 0)
    return(-1);
    
// Pointer trick: Forces the compiler to use the address of 
// buf[7] as a pointer to a float, and read the float value.
  on  = *(float *)&buf[9];
  off = *(float *)&buf[13];
  if(DEBUG){
    for (int i = 0; i<8;i++)
      Serial.print(buf[9+i], HEX);
    Serial.print('\n');
    Serial.print("On: ");
    Serial.print((int)on, DEC);
    Serial.print("Off: ");
    Serial.println((int)off, DEC);
  }
  Serial.print("Yes!");
  return(0);
}


/**
 *  @brief shifts buff one position to the left. Discarding buf[0]
**/
void discard_byte(char* buf, int len)
{
  for(int i=0;i<len-1;i++)
    buf[i] = buf[i+1];
}


/** @brief Compute the MODBUS RTU CRC
 *  @source https://ctlsys.com/support/how_to_compute_the_modbus_rtu_message_crc/
**/
unsigned int ModRTU_CRC(byte* buf, int len)
{
  unsigned int crc = 0xFFFF;
  
  for (int pos = 0; pos < len; pos++) {
    crc ^= (unsigned int)buf[pos];          // XOR byte into least sig. byte of crc
  
    for (int i = 8; i != 0; i--) {    // Loop over each bit
      if ((crc & 0x0001) != 0) {      // If the LSB is set
        crc >>= 1;                    // Shift right and XOR 0xA001
        crc ^= 0xA001;
      }
      else                            // Else LSB is not set
        crc >>= 1;                    // Just shift right
    }
  }
  // Note, this number has low and high bytes swapped, so use it accordingly (or swap bytes)
  return crc;  
}


/**
 * @brief Testfunction to vertify float behavior. 
**/
void test_float(void)
{
  float t=30.2;
  int temp;
  temp = t;
  Serial.print(temp,DEC);
}

