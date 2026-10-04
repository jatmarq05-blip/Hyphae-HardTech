//Uno R4 unpacks 4 bytes and handles F convers

#include <Wire.h>

void setup() {
  Serial.begin(9600);
  Wire.begin(); // Join I2C bus as Master
  Serial.println("R4 Master Ready");
}

void loop() {
  float tempC = 0.0;

  // 1. Ask the Nano (Address 8) for the 4 bytes that make up the float
  Wire.requestFrom(8, 4); 
  // 2. Reassemble the bytes back into the tempC variable
  if (Wire.available() == 4) {
    Wire.readBytes((char*)&tempC, 4);
    
    // 3. Instantly calculate Fahrenheit
    float tempF = (tempC * 1.8) + 32.0;

    // Output the results
    Serial.print("Zone 1 Temp: ");
    Serial.print(tempC);
    Serial.print(" °C | ");
    Serial.print(tempF);
    Serial.println(" °F");
  } else {
    Serial.println("Error: Nano 1 not responding.");
  }

Wire.requestFrom(9, 4); 
  // 2. Reassemble the bytes back into the tempC variable
  if (Wire.available() == 4) {
    Wire.readBytes((char*)&tempC, 4);
    
    // 3. Instantly calculate Fahrenheit
    float tempF = (tempC * 1.8) + 32.0;

    // Output the results
    Serial.print("Zone 2 Temp: ");
    Serial.print(tempC);
    Serial.print(" °C | ");
    Serial.print(tempF);
    Serial.println(" °F");
  } else {
    Serial.println("Error: Nano 2 not responding.");
  }
  delay(1000); 
}