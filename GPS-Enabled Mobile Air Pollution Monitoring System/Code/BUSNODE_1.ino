#include <Arduino.h>
#include <TinyGPS++.h>
#include <SoftwareSerial.h>
#include <Adafruit_AHTX0.h>
#include <SdFat.h>
#include <SPI.h>
#include <ArduinoJson.h> 

TinyGPSPlus gps;
Adafruit_AHTX0 aht;
SdFat sd;
File myFile;

const int chipSelect = PA4;

HardwareSerial gpsSerial(USART1);
HardwareSerial sdsSerial(USART2);
SoftwareSerial myGSM(PB13, PB12);

StaticJsonDocument<200> jsonDocument;  // Define a JSON document to hold the data


void setup() {
  Serial.begin(9600);
  gpsSerial.begin(9600);
  sdsSerial.begin(9600);
  myGSM.begin(9600);
  pinMode(chipSelect, OUTPUT);
  pinMode(PC13, OUTPUT);
  digitalWrite(PC13, LOW);
  pinMode(PC14 ,OUTPUT);
  digitalWrite(PC14, HIGH);
  if (!aht.begin()) {
    Serial.println("Could not find AHT? Check wiring");
  }
  delay(1000);
  Serial.println();
  Serial.println("Initializing.....");
  SD_setup();
}


void loop() {
  GPS_data();
}

//--------------------------------------------------------------------------------GPS--------------------------------------------------------------------------------------------------

String lat, lng, alt, spd, datestamp, timestamp,timestamp1,time2;
long epochTime;


void GPS_data() {
  while (gpsSerial.available() > 0) {
    char c = gpsSerial.read();
    gps.encode(c);
    if (gps.location.isUpdated()) {
      
      int year = gps.date.year();
      int month = gps.date.month();
      int day = gps.date.day();
      int hour = gps.time.hour() + 5;
      int minute = gps.time.minute() + 30;
      int hour1 = gps.time.hour();
      int minute1  = gps.time.minute();
      int second = gps.time.second();
      if (second >= 60) {
        second -= 60;
        minute++;
      }
      if (minute >= 60) {
        minute -= 60;
        hour++;
      }
      if (hour >= 24) {
        hour -= 24;
      }
      datestamp = String(day) + "-" + String(month) + "-" + String(year);
      timestamp = String(hour) + ":" + String(minute) + ":" + String(second);
      timestamp1 = String(hour1) + ":" + String(minute1) + ":" + String(second);
      time2 = datestamp + " " + timestamp1;
      epochTime = gps.date.value() + gps.time.value();
      
      Serial.println();
      Serial.println("------------------------Time & Date ---------------");
      Serial.print("Epoch Time: ");
      Serial.println(epochTime);
      Serial.print("Date (DD-MM-YYYY): ");
      Serial.println(datestamp);
      Serial.print("Time (HH-MM-SS): ");
      Serial.println(timestamp);
      Serial.println("------------------------GPS data---------------");
      Serial.print("Latitude= ");
      lat = String(gps.location.lat(), 6);
      Serial.println(lat);
      Serial.print("Longitude= ");
      lng = String(gps.location.lng(), 6);
      Serial.println(lng);
      Serial.print("Altitude (m): ");
      alt = String(gps.altitude.meters());
      Serial.println(alt);
      Serial.print("Speed (km/h): ");
      spd = String(gps.speed.kmph());
      Serial.println(spd);
      sds();
    }
  }
}

//-------------------------------------------------------------------------------------------------SDS---------------------------------------------------------------------------------
float pm25, pm10,PM25,PM10;
void sds() {
  byte buffer[10];
  bool dataReceived = false;

  // Attempt to read 10 bytes from the serial buffer
  if (sdsSerial.available() >= 10) {
    sdsSerial.readBytes(buffer, 10);
    dataReceived = true;
  }

  // Check for valid data
  if (dataReceived) {
    // Print raw data for debugging
    Serial.println("------------------------SDS data---------------");
    Serial.print("Raw Data: ");
    for (int i = 0; i < 10; i++) {
      Serial.print(buffer[i], HEX);
      Serial.print(" ");
    }
    Serial.println();
    //First Calibration
    // const double intercept_pm25 = 1.67771435;
    // const double intercept_pm10 = 19.24293753;
    // const double slope_pm25 = 1.34251743;
    // const double slope_pm10 = 1.71911605;
    // Second Calibration
    const double intercept_pm25 = 0.141772922431052;
    const double intercept_pm10 = 8.69928568936065;
    const double slope_pm25 = 1.6149007;
    const double slope_pm10 = 1.78263321;
    // Check for the correct header bytes
    if (buffer[0] == 0xAA && buffer[1] == 0xC0) {
      pm25 = (buffer[3] * 256 + buffer[2]) / 10.0;
      pm10 = (buffer[5] * 256 + buffer[4]) / 10.0;

      PM25 = (slope_pm25*pm25) + intercept_pm25;
      PM10 = (slope_pm10*pm10) + intercept_pm10;

      Serial.print("RAW PM2.5: ");
      Serial.print(pm25);
      Serial.print(" µg/m³\t");
      Serial.print("Calibrated PM2.5: ");
      Serial.print(PM25);
      Serial.println(" µg/m³\t");
      Serial.print("RAW PM10: ");
      Serial.print(pm10);
      Serial.print(" µg/m³\t");
      Serial.print("Calibrated PM10: ");
      Serial.print(PM10);
      Serial.println(" µg/m³");
    }
  }

  // Clear the serial buffer to avoid old data interference
  while (sdsSerial.available() > 0) {
    sdsSerial.read();
  }
  AHT_data();
}




//-----------------------------------------------------------------------------------------AHT-------------------------------------------------------------------------------------------
sensors_event_t humidity, temp;

float Temp, Humidity;

void AHT_data() {
  aht.getEvent(&humidity, &temp);
  Serial.println("------------------------AHT data---------------");
  Serial.print("Temperature: ");
  Temp = temp.temperature;
  Serial.print(Temp);
  Serial.println(" degrees C");
  Serial.print("Humidity: ");
  Humidity = humidity.relative_humidity;
  Serial.print(Humidity);
  Serial.println("% rH");
  Serial.println();
  SD_update();
  sendData();
}
void sendData() {

  jsonDocument["api_key"] = "BH9ZFSLLI7T0NUY9";
  jsonDocument["field1"] = PM25;
  jsonDocument["field2"] = PM10;  
  jsonDocument["field3"] = Temp;
  jsonDocument["field4"] = Humidity;
  jsonDocument["field5"] = lat.toFloat();
  jsonDocument["field6"] = lng.toFloat();
  jsonDocument["field7"] = pm25;
  jsonDocument["field8"] = pm10;
  jsonDocument["created_at"] =time2.c_str();

  // Serialize the JSON document into a string
  String jsonString;
  serializeJson(jsonDocument, jsonString);

  String request = "POST /update HTTP/1.1\r\n";
  request += "Host: api.thingspeak.com\r\n";
  request += "Content-Type: application/json\r\n";
  request += "Content-Length: " + String(jsonString.length()) + "\r\n\r\n";
  request += jsonString;
  Serial.println(request);

  myGSM.println("AT");
  delay(500);
  myGSM.println("AT+CPIN?");
  delay(500);
  // ... (other GSM commands)
  myGSM.println("AT+CREG?");
  delay(500);
  if (isNetworkRegistered()) {
    digitalWrite(PC13, HIGH); // Turn on the LED if registered
    Serial.println("Network registered.");
  } else {
    digitalWrite(PC13, LOW); // Turn off the LED if not registered
    Serial.println("Network not registered.");
  }
  delay(500);
  myGSM.println("AT+CGATT=1");
  delay(500);
  myGSM.println("AT+CIPSHUT");
  delay(500);
  myGSM.println("AT+CIPSTATUS");
  delay(500);
  myGSM.println("AT+CIPMUX=0");
  delay(500);
  ShowSerialData();
  myGSM.println("AT+CSTT=\"airtelgprs.com\"");
  delay(1000);
  ShowSerialData();
  myGSM.println("AT+CIICR");  //bring up wireless connection
  delay(1000);
  ShowSerialData();
  myGSM.println("AT+CIFSR");  //get local IP adress
  delay(1000);
  ShowSerialData();
  myGSM.println("AT+CIPSPRT=0");
  delay(1000);
  ShowSerialData();
  myGSM.println("AT+CIPSTART=\"TCP\",\"api.thingspeak.com\",\"80\"");  //start up the connection
  delay(2000);
  ShowSerialData();
  // Send the request to Thingspeak
  myGSM.println("AT+CIPSEND=" + String(request.length()));
  delay(500);
  myGSM.println(request);
  delay(500);
  ShowSerialData();
  myGSM.println("AT+CIPSHUT");  //close the connection
  delay(500);
  ShowSerialData();
}
void ShowSerialData() {
  while (myGSM.available() != 0)
    Serial.write(myGSM.read());
  delay(700);
}

bool isNetworkRegistered() {
  while (myGSM.available()) {
    String response = myGSM.readStringUntil('\n');
    if (response.indexOf("+CREG: 0,1") != -1 || response.indexOf("+CREG: 0,5") != -1) {
      // Network is registered (home or roaming)
      return true;
    }
  }

 return false;
}

//--------------------------------------------------------------------------------SD Card---------------------------------------------------------------------------------------


void SD_setup() {
  if (!sd.begin(chipSelect, SPI_HALF_SPEED)) {
    Serial.println("Card Mount Failed");
    return;
  }

  if (sd.exists("/BUS_NODE_1/DATA_NODE_1.csv")) {
    Serial.println("File exists!");
    myFile = sd.open("/BUS_NODE_1/DATA_NODE_1.csv", FILE_WRITE);
    if (myFile) {
      Serial.println("Appending data to existing file...");
    } else {
      Serial.println("Error opening existing file");
    }
  } else {
    Serial.println("File does not exist!");
    myFile = sd.open("/BUS_NODE_1/DATA_NODE_1.csv", FILE_WRITE);
    if (myFile) {
      myFile.println("Datestamp,Timestamp,CALIBRATED PM2.5,CALIBRATED PM10,RAW PM2.5,RAW PM10,Temperature(C),Humidity(%),Latitude,Longitude,Altitude,Speed");
      myFile.close();
      Serial.println("CSV file created successfully!");
    } else {
      Serial.println("Error creating CSV file");
    }
  }
}


void SD_update() {
  myFile = sd.open("/BUS_NODE_1/DATA_NODE_1.csv", FILE_WRITE);
  if (myFile) {
    myFile.print(datestamp);
    myFile.print(",");
    myFile.print(timestamp);
    myFile.print(",");
    myFile.print(PM25);
    myFile.print(",");
    myFile.print(PM10);
    myFile.print(",");
    myFile.print(pm25);
    myFile.print(",");
    myFile.print(pm10);
    myFile.print(",");
    myFile.print(Temp);
    myFile.print(",");
    myFile.print(Humidity);
    myFile.print(",");
    myFile.print(lat);
    myFile.print(",");
    myFile.print(lng);
    myFile.print(",");
    myFile.print(alt);
    myFile.print(",");
    myFile.println(spd);
    myFile.close();
    Serial.println("CSV file updated successfully!");
  } else {
    Serial.println("Error updating CSV file");
  }
}
