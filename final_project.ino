/*
Authors: Nicholas Jarvis, Colter Adams, Joseph Brea, Adesuwa Aigbe
Purpose: Final Project Swamp Cooler
Date: 4/20/2023
*/


// Download RTClib by Adafruit
#include <Wire.h>
#include "RTClib.h"



// Terminal Display  
volatile unsigned char *myUCSR0A = (unsigned char *)0x00C0; // Control Register A Status
volatile unsigned char *myUCSR0B = (unsigned char *)0x00C1; // Control Register B Status
volatile unsigned char *myUCSR0C = (unsigned char *)0x00C2; // Control Register C Status
volatile unsigned int  *myUBRR0  = (unsigned int *) 0x00C4; // Baud Rate
volatile unsigned char *myUDR0   = (unsigned char *)0x00C6; // I/O Data Register (this is the value sent to the terminal. This may be used to create different characters)
#define TBE 0x20 // Bit 5 (status bit)



// Real time clock
RTC_DS1307 rtc;

// char for printing purposes
unsigned char state;

void setup() {

  // Initially off
  state = '0';
  U0init(9600);


  if(! rtc.begin()){
    print_string("Couldn't find RTC1");
    while(1);
  }

  if(! rtc.isrunning()){
    print_string("RTC is NOT running!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }


}

void loop() {
  // put your main code here, to run repeatedly:

/*
  if(state == '0'){
    disabled();
  }
  
  else if(state == '1'){
    idle();
  }
  else if(state == '2'){
    running();
  }
  else if(state == '3'){
    error();
  }
  
*/
  change_state('1');
}


void change_state(int new_state){


  if(state != new_state){
    state = new_state;

    // State change print message
    print_string("State changed to: ");
    print_char(state);
    print_char('\n');

    // Print time
    print_time();
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




// function to initialize USART0 to "int" Baud, 8 data bits, no parity, and one stop bit. Assume FCPU = 16MHz.
// Allows for terminal output
void U0init(unsigned long U0baud)
{

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
void print_char(unsigned char c)
{

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
