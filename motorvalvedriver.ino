/**
   @brief Driver for exhaustvalve for motor. HVL project spring 2018
**/

#include <avr/io.h>
#include <avr/interrupt.h>
 

// #define DEBUG 1

#define detectorPin  2
#define outputpin    12
#define outputPinLED 13 // LED pin for test and flach for angleplate readout

void serport_handler(char d);
void print_float(float f);
void start_timer(void);
void print_menu_Serial(void);
void update_precalcvalues(void);

volatile unsigned long counts[16] = {0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0};
volatile unsigned long lastvalue = 0;
volatile unsigned int counts_idx = 0;
volatile int updated = 0;
volatile float ontime = 10.0;
volatile float offtime = 120.0;
volatile char mode = 0;   // Bad name. Is used to keep track of the outputpinstate. Redundant?
volatile uint16_t precalc_TCNT1 = 61717; // Test value
volatile uint16_t precalc_OCR1A = 64495; // Test value


/**
   @brief Interrupt routine for the interrupt line.
   @todo: rename to a more correct and descriptive name name...
**/
void measure_time() {
  unsigned long us = micros();

  digitalWrite(outputPinLED, HIGH); // Turn led ON
  start_timer();
  if (us > lastvalue) {
    counts[counts_idx] = us - lastvalue;
    counts_idx = (counts_idx + 1) % 16;
    updated = 1;
  }
  lastvalue = us;
  delayMicroseconds(250); // Flash time
  digitalWrite(outputPinLED, LOW); // Turn led ON
}


/**
   @brief Set up
   Sets up interrupt input for the detector.
   Sets up the serial port for receiving set values for the output.
**/
void setup() {
  pinMode(detectorPin, INPUT);
  pinMode(outputpin, OUTPUT);
  pinMode(outputPinLED, OUTPUT);

  attachInterrupt(0, measure_time, RISING);

  Serial.begin(115200);
  print_menu_Serial();
}


/**
   @brief Main loop. Handles serial input and update of variables according to data from last interrupt

**/
void loop() {
  if (Serial.available()) {
    serport_handler(Serial.read());
  }
  if (updated != 0) {
    update_precalcvalues(); // Update timervals
    updated = 0;
  }
}

/*
 * @brief calculates the timer values 
 * Sorry for the messy calculations. See notes to be more confused...
 */
void update_precalcvalues(void)
{
  uint32_t periodetid=0;
  float temp;
  float periodetidf;


  for(int i = 0; i<16;i++){
    periodetid += counts[i];
  }
  periodetidf = periodetid / 256; // Convert time in us to time in timer1 counts as well as devide by 16 for the average.
  
  // precalc_TCNT1:
  temp = 65536.0 - (((offtime)/360.0) * periodetidf);
  precalc_TCNT1 = (uint16_t)temp;
  
  // precalc_OCR1A:
  temp = 65536.0 - (((offtime-ontime)/360.0) * periodetidf);
  precalc_OCR1A = (uint16_t)temp;
}

/**
   @brief Prints a menu and handles simple inputs
 **/
void serport_handler(char d)
{
  static char inputbuffer[80];
  static int endindex = 0;

  inputbuffer[endindex++] = d;

  if (d == '\n') {
    Serial.print(inputbuffer);
    process_serialdata(inputbuffer);
    endindex = 0;
  }
  if (endindex == 80) {
    Serial.write("Endindex overflow!");
    endindex = 0;
  }
}


/** 
 *  @brief simple processing ascii input data
**/
void process_serialdata(char* buf)
{
  float temp;
  if (strncmp(buf, "on=", 3) == 0) {
    temp = readtime(&buf[3]);
    if (verifyinput("on", temp)) {
      ontime = temp;
    }
  }
  else if (strncmp(buf, "off=", 4) == 0) {

    temp = readtime(&buf[4]);
    if (verifyinput("off", temp)) {
      offtime = temp;
    } else if (strncmp(buf, "x", 1) == 0) {
      print_counts();
    }
  }

  print_menu_Serial();
}


/**
 * @brief Print out number of counts on serial port. Debug info
 */
void print_counts(void)
{
  uint32_t cnt=0;
  for(int i=0;i<16;i++){
    cnt += counts[i];
  }
  Serial.print("\nCounts:");
  Serial.println(cnt, DEC);
}


void print_menu_Serial(void)
{
  Serial.println("");
  Serial.println("Elmers motortest");
  Serial.println("");
  Serial.println("Nåværende verdier:");
  Serial.print(" On : ");
  print_float(ontime);
  Serial.println(" grader");
  Serial.print(" Off: ");
  print_float(offtime);
  Serial.println(" grader");
  Serial.println("");
  Serial.println("For å endre en verdi skriv f.eks:");
  Serial.println("on=123.45   eller off=234.56");
  Serial.println("For å ta i bruk nye verdier må du verifisere dem");
  Serial.println("");
}

/**
 *  @brief Reads floats from ascii string in a array
**/
float readtime(char * buf)
{
  float val;
  //char testbuf[] = "99.23";

  if (strlen(buf) < 1) {
    Serial.print("Feil lengde på inputstreng!");
    return (0.0);
  }

  val = atof(buf);

  if (val < 0.0 || val > 360.0) {
    Serial.println("Value out of range");
    return (0.0);
  }
  return (val);
}

/// @brief prints a float on the format xxx.yy
void print_float(float f) {
  int temp = f;
  Serial.print(temp);
  f = f - temp;
  f = f * 100;
  temp = f;
  Serial.print(".");
  if (temp < 10)
    Serial.print('0');
  Serial.print(temp);
}

/**
 * @brief Asks the user to verify the input data. 
 * @return 1 if verified, 0 if not.
**/
int verifyinput(char * text, float val) {
  Serial.print("Bekreft at ");
  Serial.print(text);
  Serial.print(" settes til ");
  print_float(val);
  Serial.print(" J or N?");
  while (!Serial.available());
  char reply = Serial.read();
  if ((reply == 'J') || (reply == 'j'))
    return 1;
  return 0;
}


/**
 * @brief interrupt when timer 1 has conted up to max (overflow). Disables the timer 
**/
ISR(TIMER1_OVF_vect)          // interrupt service routine that wraps a user defined function supplied by attachInterrupt
{
  TCCR1B = 0; // stop timer
  digitalWrite(outputpin, LOW); // Turn off output signal
}

ISR(TIMER1_COMPA_vect)
{
  digitalWrite(outputpin, HIGH); // Turn off output signal  
}

/**
 * @brief Handler for all non alloctated interrupts.
 */
ISR(BADISR_vect)
{
  Serial.println("Bad Interrupt catched!");
}

/**
 * @brief Sets up and starts timer 1.
 * http://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-42735-8-bit-AVR-Microcontroller-ATmega328-328P_Datasheet.pdf
**/
void start_timer(void) {

  TCNT1 = precalc_TCNT1; // 61717; // test
  OCR1A = precalc_OCR1A; // 64495;
  // From datasheet p161: TCCR1A.WGM1[3:0]=0x0
  TCCR1A = 0; // No OCR1A/OCR1B connection, Normal mode.
  TCCR1B = 0x04; // (1>>CS12); // Clock source: clk/265
  TIMSK1 = 0x03; //(1>>OCIE1A) | (1>>TOIE1); // Two interrupt sources. Outputcompare match A, and timer overflow.
}

