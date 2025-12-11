//Include Libraries
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

int count = 0;
int flag = 0;
int flag1 = 0;

#include "DHT.h"
#define DHT11PIN 33
DHT dht(DHT11PIN, DHT22);
float temp;

int Gas = 32;
int Gas_Value;

#define waterlevel 34 
int moisture;
int percentage;
int map_low = 2900;
int map_high = 900;

const int int1 = 2;
const int int2 = 4;

int led = 19;
int buzzer = 15;

//create an RF24 object
RF24 radio(12, 14, 26, 25, 27);  // CE, CSN

//address through which two modules communicate.
const byte address[6] = "00001";
boolean button_state = 0;
boolean button_state1 = 0;

void setup()
{
  while (!Serial);
    Serial.begin(9600);
    dht.begin();
  
  radio.begin();
  //set the address
  radio.openReadingPipe(0, address);  
  //Set module as receiver
  radio.startListening();

  pinMode(int1,OUTPUT);
  pinMode(int2,OUTPUT);

  pinMode(led,OUTPUT);
  pinMode(buzzer,OUTPUT);

  digitalWrite(int1, HIGH); 
  digitalWrite(int2, HIGH);
  digitalWrite(buzzer, LOW);
}

void loop()
{
  Serial.println(count);
  count = count + 1;

  //Read the data if available in buffer
    if (radio.available())
    {
    char text[32] = "";
    radio.read(&text, sizeof(text));
    Serial.println("Receiving....");
    radio.read(&button_state, sizeof(button_state));
    Serial.println(text);

    radio.read(&button_state1, sizeof(button_state1));
    Serial.println(text);
    
    if(button_state == HIGH && flag1 == 0)
    {
      digitalWrite(19, HIGH);
      Anticlockwise();
      flag1 = 1;
    }
    if(button_state1 == HIGH && flag1 == 1)
    {
      digitalWrite(19, LOW);
      Clockwise();
      flag1 = 0;
     }
     Serial.println(button_state);
     Serial.println(button_state1);
    }  
   
  Watersensor();
  Dht();
  Mq2();

if(count > 2)
{
  if(percentage > 90 && flag == 0 && flag1 == 0)
  {
    Serial.println("Water Overflow Dectected");
    Anticlockwise(); 
    flag = 1;
    flag1 = 1;
  }
 
   if(temp > 40 && Gas_Value > 2500 && flag == 0 && flag1 == 0)
  {
    Serial.println("Fire Dectected");
    Anticlockwise();
    flag = 1;
    flag1 = 1;
  } 
}
 delay(2000);
}

void Anticlockwise()
{
  digitalWrite(int1,LOW);
  digitalWrite(int2,HIGH);
  digitalWrite(buzzer,HIGH);
  Serial.println("Gate Opening");
  delay(6000);
  Stop();
 }

 void Clockwise()
{
  digitalWrite(int1,HIGH);
  digitalWrite(int2,LOW);
  digitalWrite(buzzer,HIGH);
  Serial.println("Gate Closing");
  delay(6000);
  Stop();
  flag = 0;
 }

 void Watersensor()
 {
  float moisture = analogRead(waterlevel);
  Serial.print("Raw: ");
  Serial.print(moisture);
  percentage = map(moisture, map_low, map_high, 0, 100);
  Serial.print(" | Percentage: ");
  Serial.print(percentage);
  Serial.println("%");
  delay(2000);
  }

    void Dht()
    {
    temp = dht.readTemperature();
    Serial.print("Temperature: ");
    Serial.print(temp);
    Serial.println("ºC ");
    delay(2000);
    }

   void Mq2()
   {
      Gas_Value = analogRead(Gas);
      Serial.print("Gas Sensor: ");
      Serial.print(Gas_Value);
      Serial.println("\t");
      delay(1500);
    }

   void Stop()
  {
  digitalWrite(int1,HIGH);
  digitalWrite(int2,HIGH);
  digitalWrite(buzzer,LOW);
  }
