// Uno R4 WiFi - Multi-UART Gateway
// Polls 3 Nanos over UART and forwards <TAG:value> frames to Elecrow ESP32 via TCP

#include <SoftwareSerial.h>
#include <WiFiS3.h>

// ======================= SETTINGS =======================
const char WIFI_SSID[]     = "Hello_Elecrow";
const char WIFI_PASS[]     = "12345678";
const char SERVER_IP[]     = "192.168.4.1";
const uint16_t SERVER_PORT = 22333;

WiFiClient client;
const uint32_t pollMS = 0;
float demoSpeed = 0.0;
bool useF = true;

void ensureWifi()
{
  if (WiFi.status()==WL_CONNECTED)
  return;
  static uint32_t lastTry = 0;
  if (millis()-lastTry < 5000)
  return;
  lastTry = millis();

  client.stop();
  Serial.println("WiFi Connection Lost... Reconnecting");
  WiFi.begin(WIFI_SSID,WIFI_PASS);
}

void ensureTCP()
{
  if(client.connected())
  return;
  static uint32_t lastTry = 0;
  if(millis() - lastTry <2000)
  return;
  lastTry = millis();

  client.stop();
  Serial.println("Elecrow Connection Lost... Reconnecting");
  client.connect(SERVER_IP, SERVER_PORT);
}

String askNano(Stream &port, const char* reqCmd)
{
  while(port.available())
  port.read();
  port.println(reqCmd);

  uint32_t start = millis();
  while(!port.available() && (millis() - start <100)){}

  if(!port.available())
  return "";

  String reply = port.readStringUntil('\n');
  reply.trim();
  return reply;
}

String askSoftNano(uint8_t rxPin, uint8_t txPin, const char* reqCmd)
{
  SoftwareSerial soft(rxPin, txPin);
  soft.begin(9600);
  String reply = askNano(soft,reqCmd);
  soft.end();
  return reply;
}

void checkElecrowCommands() {
  if (client && client.available()) {
    String cmd = client.readStringUntil('\n');
    cmd.trim();

    if (cmd.indexOf("<UNIT_C>") != -1) {
      useF = false;
      Serial.println("Switched to Celsius");
    } 
    else if (cmd.indexOf("<UNIT_F>") != -1) {
      useF = true;
      Serial.println("Switched to Fahrenheit");
    }
  }
}

void setup()
{
  Serial.begin(115200);
  Serial1.begin(9600); // Nano 1
  //--Nano 2 TX--
  pinMode(3,OUTPUT);
  digitalWrite(3,HIGH);
  //--------------
  //--Nano 3 TX--
  pinMode(9,OUTPUT);
  digitalWrite(9,HIGH);
  //--------------
  WiFi.begin(WIFI_SSID, WIFI_PASS);
}

void loop()
{
  ensureWifi();
  if(WiFi.status() == WL_CONNECTED)
  ensureTCP();

  //future Cel of Far button press.
  checkElecrowCommands;
  
  if(millis()-pollMS < 1000)
  return;

  //picking command based off of current setting C/F
  const char* tempCmd = useF ? "<REQ_F>" : "<REQ_C>";
  String frameN1 = askNano(Serial1,tempCmd);
  String frameN2 = askSoftNano(2,3,"<REQ>");
  String frameN3 = askSoftNano(8,9,"<REQ>");

  demoSpeed += 1.0;
  if (demoSpeed > 60.0) demoSpeed = 0.0;
  String speedTag = "<S_EXT:" + String(demoSpeed, 2) + ">";

  String fullFrame = speedTag + frameN1 + frameN2 + frameN3;

  if (client.connected() && fullFrame.length() > 0) {
    client.print(fullFrame);
  }

  Serial.print(useF ? "[F Mode] " : "[C Mode] ");
  Serial.println("Sent -> " + fullFrame);
}
