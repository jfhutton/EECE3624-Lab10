/*
  Simple character read.  This is looking for an "H" or and "L"
  and will set the LED tied to the given pin on or off.

  "SoftwareSerial" is a library that does a bit-bang USART interface on 
  any two digital GPIOs on an arduino.  This allows you to leave the 
  primary USART for debugging.

  This routine setups a USART on pins 2 and 4 named "bluetooth".  It then
  setups up this uart at 9600 baud.  In the loop, it checks to see if a
  character has arrived, and then reads it.

  The LED13 (built in LED) is blinked to show the program is running.  This
  blink can be moved inside the bluetooth if statement for debugging, or
  the normal serial pipe can be used.
*/

#include <SoftwareSerial.h>

//receiver
int led = 13; 
int incomingByte;     

// HC-05
SoftwareSerial bluetooth(2,4);  // RX, TX

void setup() {
  Serial.begin(9600);
  pinMode(led, OUTPUT);
  // Setup the HC-05 bluetooth device
  // (HC)=(Uno).  VCC=5V, GND=GND, TX=RX(pin2),
  // RX=DNC! (possible damage if 5V Uno TX drives to HC-05)
  bluetooth.begin(9600);
}

void loop() {
  digitalWrite(led,HIGH);
  delay(1000);
  digitalWrite(led,LOW);
  delay(1000);
  if (bluetooth.available()) {  
    incomingByte = bluetooth.read();
    Serial.println(incomingByte);
  }
}
