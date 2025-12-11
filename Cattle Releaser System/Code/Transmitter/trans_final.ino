#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
RF24 radio(12, 14, 26, 25, 27); 
const byte address[6] = "00001";     
int button_pin = 2;
int button_pin1 = 4;
boolean button_state = 0;
boolean button_state1 = 0;
int flag = 0;
int flag1 = 0;


void setup() 
{
pinMode(button_pin, INPUT);
pinMode(button_pin1, INPUT);
radio.begin();                 
radio.openWritingPipe(address);
radio.stopListening();  
Serial.begin(9600);
}


void loop()
{
button_state = digitalRead(button_pin);
Serial.println(button_state);

button_state1 = digitalRead(button_pin1);
Serial.println(button_state1);

if(button_state == HIGH && flag == 0)
{
const char text[] = "Your Button 1 State is HIGH";
radio.write(&text, sizeof(text));   
Serial.println(text);
flag = 1;             
}
radio.write(&button_state, sizeof(button_state)); 

if(button_state1 == HIGH && flag == 1)
{
const char text[] = "Your Button 2 State is HIGH";
radio.write(&text, sizeof(text));   
Serial.println(text);
flag = 0;             
}
radio.write(&button_state1, sizeof(button_state1)); 


delay(2000);
}
