#include <Wire.h>
#include <max6675.h>

// SPI pins for Thermocouple 1
const int thermo1SO = 4;
const int thermo1CS = 5;
const int thermo1SCK = 6;

// SPI pins for Thermocouple 2
const int thermo2SO = 7;
const int thermo2CS = 8;
const int thermo2SCK = 9;

MAX6675 thermocouple1(thermo1SCK, thermo1CS, thermo1SO);
MAX6675 thermocouple2(thermo2SCK, thermo2CS, thermo2SO);

// Array to hold both temperatures (Index 0 = Temp 1, Index 1 = Temp 2)
float currentTemps[2] = {0.0, 0.0};

void setup() {
  Serial.begin(9600);           // Optional: For PC debugging
  Wire.begin(9);                // Join I2C bus at address 9 
  Wire.onRequest(requestEvent); // Wait for the R4 to ask
  
  Serial.println("Nano 9 Booted. Reading 2 Thermocouples...");
}

void loop() {
  currentTemps[0] = thermocouple1.readCelsius();
  currentTemps[1] = thermocouple2.readCelsius();

  // Print locally to PC just to verify the sensors work
  // Serial.print("Nano 9 Local T1: "); Serial.print(currentTemps[0]);
  // Serial.print(" | T2: "); Serial.println(currentTemps[1]);

  delay(250); // MAX6675 requires at least 250ms between reads
}

void requestEvent() {
  // Send the entire array (both floats = 8 bytes) in one single I2C payload
  Wire.write((byte*)currentTemps, sizeof(currentTemps));
}