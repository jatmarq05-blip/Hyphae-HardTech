#include <SoftwareSerial.h>
#include <WiFiS3.h>

const char WIFI_SSID[]     = "Hello_Elecrow";
const char WIFI_PASS[]     = "12345678";
const char SERVER_IP[]     = "192.168.4.1";
const uint16_t SERVER_PORT = 22333;

WiFiClient client;
uint32_t pollMS = 0;
float demoSpeed = 0.0;
bool useF = true;

void ensureWifi() {
  if (WiFi.status() == WL_CONNECTED) return;
  static uint32_t lastTry = 0;
  if (millis() - lastTry < 5000) return;
  lastTry = millis();
  client.stop();
  Serial.println("WiFi Link Down... Initiating connection");
  WiFi.begin(WIFI_SSID, WIFI_PASS);
}

void ensureTCP() {
  if (WiFi.status() != WL_CONNECTED) return;
  if (client.connected()) return;
  static uint32_t lastTry = 0;
  if (millis() - lastTry < 3000) return;
  lastTry = millis();
  client.stop();
  Serial.println("Connecting to Elecrow TCP Server...");
  if (client.connect(SERVER_IP, SERVER_PORT)) {
    Serial.println("TCP Connection Established!");
  } else {
    Serial.println("TCP Connection Failed (Will retry in 3s)");
  }
}

String askNano(Stream &port, const char* reqCmd) {
  int junkCount = 0;
  while (port.available() && junkCount < 50) {
    port.read();
    junkCount++;
  }

  port.println(reqCmd);

  String reply = "";
  uint32_t safetyCounter = 0;
  while (safetyCounter < 500000) { 
    if (port.available()) {
      char c = port.read();
      if (c == '\n') break;
      reply += c;
      if (reply.length() > 40) break;
    }
    safetyCounter++;
  }

  reply.trim();
  return reply;
}

void checkElecrowCommands() {
  if (client.connected() && client.available()) {
    String cmd = client.readStringUntil('\n');
    cmd.trim();
    if (cmd.indexOf("<UNIT_C>") != -1) {
      useF = false;
      Serial.println("Switched to Celsius");
    } else if (cmd.indexOf("<UNIT_F>") != -1) {
      useF = true;
      Serial.println("Switched to Fahrenheit");
    }
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("Uno R4 Booting - Initializing UARTs...");

  
  Serial1.begin(9600);   

  Serial.println("Attempting initial WiFi connection...");
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.println("Setup complete. Entering loop.");
}

void loop() {
  ensureWifi();
  ensureTCP();
  checkElecrowCommands();

  if (millis() - pollMS < 1000) return;
  pollMS = millis();

  const char* tempCmd = useF ? "<REQ_F>" : "<REQ_C>";

  
  String frameN1 = askNano(Serial1, tempCmd);

  
  String frameN2 = "";
  { 
    SoftwareSerial portN2(2, 3);
    portN2.begin(9600);
    frameN2 = askNano(portN2, "<REQ>");
    portN2.end();
  } 

  // 3. Nano 3 (Strict Local Scope)
  String frameN3 = "";
  { 
    SoftwareSerial portN3(8, 9);
    portN3.begin(9600);
    frameN3 = askNano(portN3, "<REQ>");
    portN3.end();
  }

  demoSpeed += 1.0f;
  if (demoSpeed > 60.0f) demoSpeed = 0.0f;

  if (client.connected()) {
    client.print("<S_EXT:");
    client.print(demoSpeed, 2);
    client.print(">");

    if (frameN1.length() > 0) client.print(frameN1);
    if (frameN2.length() > 0) client.print(frameN2);
    if (frameN3.length() > 0) client.print(frameN3);
  }

  Serial.print(useF ? "[F Mode] " : "[C Mode] ");
  Serial.print("Polled -> ");
  Serial.print("N1: "); Serial.print(frameN1.length() > 0 ? frameN1 : "TIMEOUT ");
  Serial.print(" | N2: "); Serial.print(frameN2.length() > 0 ? frameN2 : "TIMEOUT ");
  Serial.print(" | N3: "); Serial.println(frameN3.length() > 0 ? frameN3 : "TIMEOUT");
}