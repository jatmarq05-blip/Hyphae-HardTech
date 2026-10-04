#include <Wire.h>
#include <max6675.h>

// SPI pins for the single thermocouple
const int thermoSO = 4;
const int thermoCS = 5;
const int thermoSCK = 6;

MAX6675 thermocouple(thermoSCK, thermoCS, thermoSO);

float currentTemp = 0.0;

void setup() {
  Serial.begin(9600);           // Optional: For PC debugging
  Wire.begin(8);                // Join I2C bus at address 8
  Wire.onRequest(requestEvent); // Wait for the R4 to ask
  
  Serial.println("Nano 8 Booted. Reading 1 Thermocouple...");
}

void loop() {
  currentTemp = thermocouple.readCelsius();
  
  // Print locally to PC just to verify the sensor works
  // Serial.print("Nano 8 Local Temp: ");
  // Serial.println(currentTemp);
  
  delay(250); // MAX6675 requires at least 250ms between reads
}

void requestEvent() {
  // Send the single float (4 bytes) over I2C
  Wire.write((byte*)&currentTemp, sizeof(currentTemp));
}