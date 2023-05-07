/*
Authors: Nicholas Jarvis, Colter Adams, Joseph Brea, Adesuwa Aigbe
Purpose: Final Project Swamp Cooler
Date: 4/20/2023
*/

// Download RTClib by Adafruit
// Download DHTLib by Rob Tillaart
// Download LiquidCrystal by Arduino, Adafruit
#include <Wire.h>
#include "RTClib.h"
#include <Stepper.h>
#include <LiquidCrystal.h>
#include "dht.h"

// States
void disabled();
void idle();
void running();
void error();
void change_state(unsigned char);

// Analog
void adc_init();
unsigned int adc_read(unsigned char);

// Devices
void change_stepper_direction();
bool check_water_level();
bool check_temp();
void toggle_fan(bool);
void display_LCD(float, float);

// Breadboard Utility
bool reset_button();
bool stop_button();
void led_toggle();

// Terminal Display
void print_time();
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

// LEDs  (PK0, PK1, PK2, PK3; Pin A10, Pin A9, Pin A11, Pin A8)
// Buttons (Pk4, PK5, PK6; Pin A12, Pin A13, Pin A14)
volatile unsigned char* port_k = (unsigned char*) 0x108;
volatile unsigned char* ddr_k = (unsigned char*) 0x107;
volatile unsigned char* pin_k = (unsigned char*) 0x106;

// Fan Motor (PF0, PF1, PF4; A0, A1, A4) (speed pin is A4)
// Water Sensor (PF2, PF3; A2, A3)
volatile unsigned char *port_f = (unsigned char *) 0x31;
volatile unsigned char *ddr_f = (unsigned char *) 0x30;
volatile unsigned char *pin_f = (unsigned char *) 0x2F;

// Analog
volatile unsigned char* my_ADMUX = (unsigned char*) 0x7C;
volatile unsigned char* my_ADCSRB = (unsigned char*) 0x7B;
volatile unsigned char* my_ADCSRA = (unsigned char*) 0x7A;
volatile unsigned int* my_ADC_DATA = (unsigned int*) 0x78;

// Stepper motor
const int stepsPerRevolution = 2038;
Stepper my_stepper = Stepper(stepsPerRevolution, 52, 48, 50, 46);

// Real time clock
RTC_DS1307 rtc;

// LCD
LiquidCrystal lcd(8, 7, 6, 5, 4, 3);

// DHT
dht DHT;

// char for printing purposes
unsigned char state;

void setup() {
  // Initially off
  state = '0';

  U0init(9600);
  
  rtc.begin();
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  
  // Set up analog
  adc_init();

  // Set up stepper motor
  my_stepper.setSpeed(200);  

  // DHT
  int chk = DHT.read11(2);  
  
  lcd.begin(16, 2);
  
  // LEDs in output mode
  *ddr_k |= 0b00001111;

  // Buttons in input mode
  *ddr_k &= 0b01110000;

  // Water Sensor power pin in output mode
  // Fan in output mode (pre adding A4)
  *ddr_f |= 0b00010111;

  // Water Sensor data pin in input mode
  *ddr_f &= ~(0x01 << 3);

  // Fan starts off
  *port_f &= 0b11111110;
}

void loop() {

  // LED starts on
  led_toggle();

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
}

void disabled(){
  // No display
  lcd.clear();

  // Stays disabled
  while (state == '0'){

    // Change to idle
    if (reset_button() == true){
      change_state('1');
    }
  }
}

void idle(){

  // Fan always off
  toggle_fan(false);

  // Stay idle
  while (state == '1'){

    // Change vent angle
    change_stepper_direction();

      // High enough temp
      if (check_temp() == true){

        // Change to running
        change_state('2');
      }

    // Check water level
    if (check_water_level() == true){

      // Change to error state
      change_state('3');
    }

    // Check stop button
    if (stop_button() == true){

      // Fan off
      toggle_fan(false);

      // Change to disabled
      change_state('0');
    }
  }
}

void running(){

  // Fan on
  toggle_fan(true);

  // Stay in running
  while (state == '2'){

    // Change vent angle
    change_stepper_direction();

    // Low temperature
    if (check_temp() == false){

      // Change to idle
      toggle_fan(false);
      change_state('1');
    }

    // Check water level
    if (check_water_level() == true){

      // Change to error 
      toggle_fan(false);
      change_state('3');
    }

    // Check stop button
    if (stop_button() == true){

      // Change to disabled
      toggle_fan(false);
      change_state('0');
    }
  }
}

void error(){

  // Fan off
  toggle_fan(false);

  // Error LCD display
  lcd.setCursor(0,0);
  lcd.print("ERROR:    ");
  lcd.setCursor(0,1);
  lcd.print("Water low    ");

  // Stay in error
  while (state == '3'){

    // Check vent angle
    change_stepper_direction();
    
    // Check reset button and water level
    if (reset_button() == true && check_water_level() == false){

      // Change to idle
      change_state('1');
    }

    // Check stop button
    if (stop_button() == true){
      
      // Change to disabled
      toggle_fan(false);
      change_state('0');
    }
  }
}

void change_state(unsigned char new_state){

  if(state != new_state){

    // State change print message
    print_string("State changed to: ");
    print_char(new_state);
    print_char(' ');

    // Print time
    print_time();
    
    // Toggle light
    led_toggle();

    state = new_state;
  }
}

void print_time(){

  // Set exact time
  DateTime current_time = rtc.now();

  // Print 
  char buf[] = "hh:mm:ss";
  print_string("at ");
  print_string(current_time.toString(buf));

  print_char('\n');
}

void change_stepper_direction(){

  // Hold down button to adjust direction
  if(*pin_k & (0x01 << 6)){

    print_string("Vent angle changed");
    print_char('\n');

    // Rotate Direction in increments
    my_stepper.step(45);
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

  // Turn power on
  *port_f |= (0x01 << 2);

  // Read
  unsigned volatile level = adc_read(3);

  // Turn power off
  *port_f &= ~(0x01 << 2);

  // Too low of water level
  if(level <= 104){

    // Too low
    return true;
  }
  
  // Not low
  return false;
}

bool check_temp(){

  // Display
  display_LCD(DHT.humidity, DHT.temperature);

  if (DHT.temperature > 19){
    return true;
  }
  else{
    return false;
  }
}

void led_toggle(){

  // Turn all off
  *port_k &= 0b00000000;

  // Turn on
  switch(state){

    // Disabled - Yellow
    case '0':
      *port_k |= 0b00000010;
    break;

    // Idle - Green
    case '1':
    // LED on
      *port_k |= 0b00001000;
    break;

    // Running - Blue
    case '2':
      *port_k |= 0b00000001;
    break;

    // Error - Red
    case '3':
      *port_k |= 0b00000100;
    break;    
  }
}

void toggle_fan(bool on){
  // On -> off
  if(on == true){
    *port_f |= 0b00010010;
  }

  // Off -> on
  else{
    *port_f &= 0b11101111;
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
    if(*pin_k & (0x01 << 5)){
    // Pressed
    return true;
  }
  else{
    // Not pressed
    return false;
  }
}

bool stop_button(){

  // Button
    if(*pin_k & (0x01 << 4)){
      
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