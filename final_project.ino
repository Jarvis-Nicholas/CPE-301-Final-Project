/*
Authors: Nicholas Jarvis, Colter Adams, Joseph Brea, Adesuwa Aigbe
Purpose: Final Project Swamp Cooler
Date: 4/20/2023
*/


// Download RTClib by Adafruit
// Download DHTLib by Rob Tillaart
// Download LiquidCrystal by Arduino, Adafruit
//#include <Wire.h>
#include "RTClib.h"
#include <Stepper.h>
#include <LiquidCrystal.h>
#include <dht.h>

// Function declarations
void disabled();
void idle();
void running();
void error();
void change_state(unsigned char);
void print_time();
void change_stepper_direction();
void adc_init();
unsigned int adc_read(unsigned char);
bool check_water_level();
void led_toggle();
void display_LCD(float, float);
bool reset_button();
bool stop_button();
void U0init(unsigned long);
void print_char(unsigned char);
void print_string(String);

// Terminal Display  
volatile unsigned char *myUCSR0A = (unsigned char *)0x00C0; // Control Register A Status
volatile unsigned char *myUCSR0B = (unsigned char *)0x00C1; // Control Register B Status
volatile unsigned char *myUCSR0C = (unsigned char *)0x00C2; // Control Register C Status
volatile unsigned int  *myUBRR0  = (unsigned int *) 0x00C4; // Baud Rate
volatile unsigned char *myUDR0   = (unsigned char *)0x00C6; // I/O Data Register (this is the value sent to the terminal. This may be used to create different characters)
#define TBE 0x20 // Bit 5 (status bit)

// Stepper Motor button??
// (PE3, 5)
volatile unsigned char *pin_e = (unsigned char *) 0x20;
volatile unsigned char *ddr_e = (unsigned char *) 0x21;
volatile unsigned char *port_e = (unsigned char *) 0x22;

// Analog
volatile unsigned char* my_ADMUX = (unsigned char*) 0x7C;
volatile unsigned char* my_ADCSRB = (unsigned char*) 0x7B;
volatile unsigned char* my_ADCSRA = (unsigned char*) 0x7A;
volatile unsigned int* my_ADC_DATA = (unsigned int*) 0x78;

// Stepper motor
const int stepsPerRevolution = 2038;
Stepper my_stepper = Stepper(stepsPerRevolution, 7, 5 ,6, 4);



// Real time clock
RTC_DS1307 rtc;

// LCD
LiquidCrystal lcd(8, 7, 6, 5, 4, 3);

// DHT
DHT dht(2, DHT11);

// char for printing purposes
unsigned char state;

void setup() {

  // Initially off
  state = '0';
  U0init(9600);

  // Set up analog
  adc_init();

  // Set up stepper motor
  my_stepper.setSpeed(5);


  // Stepper button?? (input mode)??
  *ddr_e &= ~(0x01 << 2);

  /*
  if(! rtc.begin()){
    print_string("Couldn't find RTC1");
    while(1);
  }

  if(! rtc.isrunning()){
    print_string("RTC is NOT running!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  */


}

void loop() {


  // LED starts on
  // led_toggle();
/*
  switch(state){
    case '0':
      disabled();
    break;
    case '1':
      idle();
    break;
    case '2':
      running();
    break;
    case '3':
      error();
    break;    
  }
*/
  print_time();
  change_state('1');
}


void change_state(unsigned char new_state){

  if(state != new_state){
    state = new_state;

    // State change print message
    print_string("State changed to: ");
    print_char(state);
    print_char('\n');

    // Print time
    //print_time();
    
    // Toggle light
    // led_toggle(state);
  }
}


void print_time(){

  // Set exact time
  DateTime current_time = rtc.now();

  // Print 
  char buf[] = "hh:mm:ss";
  print_string("at ");
  print_string(current_time.toString(buf));
  print_string(":");

  print_string(":");
  print_char('\n');
  delay(1000);
}

// Change Stepper motor direction
void change_stepper_direction(){

  // Wait for button input??
  if((*pin_e & 0b00000010) == 0b00000010){

    print_string("Vent angle changed ");
    //printTime();

    // Change direction
    //my_stepper.step(-stepsPerRevolution);

    // Rotate Direction in increments of 5
    my_stepper.step(5);
  }
}




// Analog setup
void adc_init(){

  // setup the A register
  *my_ADCSRA |= 0b10000000; // set bit   7 to 1 to enable the ADC
  *my_ADCSRA &= 0b11011111; // clear bit 6 to 0 to disable the ADC trigger mode
  *my_ADCSRA &= 0b11110111; // clear bit 5 to 0 to disable the ADC interrupt
  *my_ADCSRA &= 0b11111000; // clear bit 0-2 to 0 to set prescaler selection to slow reading
  
  // setup the MUX Register
  *my_ADMUX  &= 0b01111111; // clear bit 7 to 0 for AVCC analog reference
  *my_ADMUX  |= 0b01000000; // set bit   6 to 1 for AVCC analog reference
  *my_ADMUX  &= 0b11011111; // clear bit 5 to 0 for right adjust result
  *my_ADMUX  &= 0b11100000; // clear bit 4-0 to 0 to reset the channel and gain bits
}

// Analog read
unsigned int adc_read(unsigned char adc_channel_num){

  // clear the channel selection bits (MUX 4:0)
  *my_ADMUX  &= 0b11100000;
  
  // clear the channel selection bits (MUX 5)
  *my_ADCSRB &= 0b11110111;
  
  // set the channel number
  if(adc_channel_num > 7 )
  {
    // set the channel selection bits, but remove the most significant bit (bit 3)
    adc_channel_num -= 8;
    
    // set MUX bit 5
    *my_ADCSRB |= 0b00001000;
  }
  
  // set the channel selection bits
  *my_ADMUX  += adc_channel_num;
  
  // set bit 6 of ADCSRA to 1 to start a conversion
  *my_ADCSRA |= 0x40;
  
  // wait for the conversion to complete
  while((*my_ADCSRA & 0x40) != 0);
  
  // return the result in the ADC data register
  return *my_ADC_DATA;
}

bool check_water_level(){

  // If water level < 130
  if(adc_read(0) < 130){

    // Too low
    return true;
  }

  // Not low
  return false;
}

void led_toggle(){
  switch(state){
    case '0':
    
    break;
    case '1':
    
    break;
    case '2':
    
    break;
    case '3':
    
    break;    
  }
}

void display_LCD(float top, float bottom){
  lcd.setCursor(0,0);
  lcd.print("Hum: ");
  lcd.print(top);
  lcd.setCursor(0,1);
  lcd.print("Temp: ");
  lcd.print(bottom);
}


bool reset_button(){

  // Button
  if((*pinb & 0b00000001) == 0b00000001)
  {
    // Pressed
    return true;
  }
  // Not pressed
  return false;
}

bool stop_button(){

  // Button
  if((*pinb & 0b00000010) == 0b00000010)
  {
    // Pressed
    return true;
  }

  // Not pressed
  return false;
}


// function to initialize USART0 to "int" Baud, 8 data bits, no parity, and one stop bit. Assume FCPU = 16MHz.
// Allows for terminal output
void U0init(unsigned long U0baud){

 unsigned long FCPU = 16000000;
 unsigned int tbaud;
 tbaud = (FCPU / 16 / U0baud - 1);
 // Same as (FCPU / (16 * U0baud)) - 1;
 *myUCSR0A = 0x20;
 *myUCSR0B = 0x18;
 *myUCSR0C = 0x06;
 *myUBRR0  = tbaud;
}


// Wait for USART0 TBE to be set (1) then write character to transmit buffer
// Print char
void print_char(unsigned char c){

  // Loop until TBE flag (bit 5) == 1 -> (AND makes 1)
  while(!(*myUCSR0A & TBE)); // while bit != 1, keep running

  // TBE is now 1, so write value to terminal
  *myUDR0 = c;
}

void print_string(String input){

  for(int i = 0; i < input.length(); i++){
    print_char(input[i]);
  }
}



ISR(TIMER1_OVF_vect){
  state = 1;
}