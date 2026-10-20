// Nano 1 - 3x MAX6675, answers requests from the Uno over hardware UART
#include <max6675.h>

const int thermo1SO = 4,  thermo1CS = 5,  thermo1SCK = 6;
const int thermo2SO = 7,  thermo2CS = 8,  thermo2SCK = 9;
const int thermo3SO = 10, thermo3CS = 11, thermo3SCK = 12;

MAX6675 thermocouple1(thermo1SCK, thermo1CS, thermo1SO);
MAX6675 thermocouple2(thermo2SCK, thermo2CS, thermo2SO);
MAX6675 thermocouple3(thermo3SCK, thermo3CS, thermo3SO);

void setup()
{
  Serial.begin(9600);
  Serial.setTimeout(50);     // don't wait a full second for a newline
}

// Send "<TAG:value>" only if the reading is valid (NaN = fault, so skip it).
void sendTemp(const char* tag, float value)
{
  if (isnan(value)) return;
  Serial.print('<');
  Serial.print(tag);
  Serial.print(':');
  Serial.print(value, 2);
  Serial.print('>');
}

void loop()
{
  if (Serial.available())
  {
    String cmd = Serial.readStringUntil('\n'); 
    cmd.trim();

    if (cmd == "<REQ_F>" || cmd == "<REQ_C>")
    {
      bool useF = (cmd == "<REQ_F>");

      sendTemp("T_EX1", useF ? thermocouple1.readFahrenheit() : thermocouple1.readCelsius());
      sendTemp("T_EX2", useF ? thermocouple2.readFahrenheit() : thermocouple2.readCelsius());
      sendTemp("T_FC",  useF ? thermocouple3.readFahrenheit() : thermocouple3.readCelsius());

      Serial.println();   // end of reply
    }
  }
}