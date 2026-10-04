#include <Wire.h>

float currentScrewSpeed = 0.0;

void setup() {
  Serial.begin(115200);   // PC Monitor for debugging
  Serial1.begin(115200);  // ESP32 Screen Connection
  Wire.begin();           // Join I2C bus as Master
  
  Serial.println("Uno R4 Trial Booted. Polling Nanos...");
}

void loop() {
  // Temporary variables for raw Celsius data
  float t1_C = 0.0;
  float t2_C = 0.0;
  float fc_C = 0.0;

  // Variables for converted Fahrenheit data
  float t1_F = 0.0;
  float t2_F = 0.0;
  float fc_F = 0.0;

  // --- Poll Nano 1 (Address 8) - 1 Thermocouple ---
  Wire.requestFrom(8, 4); // Ask for 4 bytes (1 float)
  if (Wire.available() == 4) {
    Wire.readBytes((byte*)&t1_C, 4);
    t1_F = (t1_C * 1.8) + 32.0;
  }

  // --- Poll Nano 2 (Address 9) - 2 Thermocouples ---
  Wire.requestFrom(9, 8); // Ask for 8 bytes (2 floats)
  if (Wire.available() == 8) {
    float incomingTemps[2];
    Wire.readBytes((byte*)incomingTemps, sizeof(incomingTemps)); // Read all 8 bytes into array
    
    // Break the array into individual variables
    t2_C = incomingTemps[0];
    fc_C = incomingTemps[1];
    
    t2_F = (t2_C * 1.8) + 32.0;
    fc_F = (fc_C * 1.8) + 32.0;
  }

  // --- Demo Math for Screw Speed ---
  currentScrewSpeed += 1.0;
  if (currentScrewSpeed > 60.0) {
    currentScrewSpeed = 0.0;
  }

  // --- TRANSMIT TO ESP32 ---
  // Note: Using print() instead of println() to prevent hidden line breaks
  
  // 1. Screw Speed
  Serial1.print("<S_EXT:");
  Serial1.print(currentScrewSpeed);
  Serial1.print(">");

  // 2. Extruder Temp 1 (From Nano 8)
  Serial1.print("<T_EX1:");
  Serial1.print(t1_F);
  Serial1.print(">");

  // 3. Extruder Temp 2 (From Nano 9, Sensor 1)
  Serial1.print("<T_EX2:");
  Serial1.print(t2_F);
  Serial1.print(">");

  // 4. Feed Cooling Temp (From Nano 9, Sensor 2)
  Serial1.print("<T_FC:");
  Serial1.print(fc_F);
  Serial1.print(">");

  // --- Print to PC Monitor for visual debugging ---
  Serial.print("Sent -> T1: "); Serial.print(t1_F);
  Serial.print(" | T2: "); Serial.print(t2_F);
  Serial.print(" | FC: "); Serial.println(fc_F);

  delay(1000); // Wait 1 second before polling again
}