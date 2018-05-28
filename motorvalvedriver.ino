/**
   @brief Driver for exhaustvalve for motor. HVL project spring 2018



**/

// #define DEBUG 1

#define detectorPin  2
#define outputpin    12
#define outputPinLED 13 // LED pin for test and flach for angleplate readout


void serport_handler(char d);
void print_float(float f);


volatile unsigned long counts[16] = {0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0};
volatile unsigned long lastvalue = 0;
volatile unsigned int counts_idx = 0;
volatile int updated = 0;
volatile float ontime = 0;
volatile float offtime = 123.4;
volatile char mode = 0;   // Bad name. Is used to keep track of the outputpinstate. Redundant?
/**
   @brief Interrupt routine for the interrupt line.
**/
void measure_time() {
  unsigned long us = micros();
  // load and start timer
  digitalWrite(outputPinLED, HIGH); // Turn led ON
  reset_timer();
  if (us > lastvalue) {
    counts[counts_idx] = us - lastvalue;
    counts_idx = (counts_idx + 1) % 16;
    updated = 1;
  }
  lastvalue = us;
  delayMicroseconds(100); // Flash time
  digitalWrite(outputPinLED, LOW); // Turn led ON

}


/**
   @brief Set up
   Sets up interrupt input for the detector.
   Sets up the serial port for receiving set values for the output.
**/
void setup() {
  pinMode(detectorPin, INPUT);

  attachInterrupt(0, measure_time, RISING);

  Serial.begin(115200);
  Timer1.initialize(2200);
  Timer1.stop();
  print_menu_Serial();
}


/**
   @todo Add Debugoutput on serial port

**/
void loop() {
  if (Serial.available()) {
    serport_handler(Serial.read());
  }
  if (updated != 0) {
    // Update timervals
  }
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


/** @brief simple processing of "semi modbus" message
    @returns -1 on error, 0 on success
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
    }
  }

  print_menu_Serial();
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

/// @brief prints a float on the format xxx.y
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

void timer1_isr(void)
{
  if (mode == 0) {
    digitalWrite(outputpin, HIGH);
    // load timer
  } else {
    digitalWrite(outputpin, LOW);
    // stop timer
  }
}

void reset_timer(void) {

}

