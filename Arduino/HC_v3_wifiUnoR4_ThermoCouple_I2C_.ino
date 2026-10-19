// Uno R4 WiFi - sensor gateway (v2)
// Polls two Nanos over I2C and sends <TAG:value> frames to the Elecrow ESP32 over TCP.
// Board: Arduino UNO R4 WiFi

#include <Wire.h>
#include <WiFiS3.h>

// ======================= SETTINGS =======================
// Must match the Elecrow sketch exactly.
const char WIFI_SSID[]   = "Hello_Elecrow";
const char WIFI_PASS[]   = "12345678";
const char SERVER_IP[]   = "192.168.4.1";
const uint16_t SERVER_PORT = 22333;

const uint8_t ADDR_NANO1 = 8;    // sends 1 float: T1 (C)
const uint8_t ADDR_NANO2 = 9;    // sends 2 floats: T2 (C), FC (C)

const uint32_t POLL_INTERVAL_MS = 1000;   // sensor read + send period
const uint32_t TCP_RETRY_MS     = 2000;   // wait between TCP connect attempts
const uint32_t WIFI_RETRY_MS    = 5000;   // wait between WiFi rejoin attempts
const uint32_t WIFI_SETUP_WAIT_MS = 20000; // how long setup() waits for WiFi

const float TEMP_MIN_C = -50.0;   // readings outside this range are treated as faults
const float TEMP_MAX_C = 600.0;

// 1 = send a fake screw speed (bench testing only). 0 = do not send S_EXT at all.
// WARNING: leave this at 0 on a real machine, or the screen shows invented data.
#define USE_DEMO_SPEED 1
// ========================================================

WiFiClient client;
float demoScrewSpeed = 0.0;

// Read `count` floats from an I2C peripheral. Returns true only if the full read
// arrived and every value is a sane number.
bool readFloats(uint8_t addr, float* out, uint8_t count)
{
  const int bytes = count * (int)sizeof(float);

  Wire.requestFrom((int)addr, bytes);
  if (Wire.available() != bytes) {
    while (Wire.available()) Wire.read();   // drain leftovers so the next read stays aligned
    return false;
  }

  Wire.readBytes((byte*)out, bytes);

  for (uint8_t i = 0; i < count; i++) {
    if (isnan(out[i]) || out[i] < TEMP_MIN_C || out[i] > TEMP_MAX_C) return false;
  }
  return true;
}

float toF(float c) { return (c * 1.8f) + 32.0f; }

// Append "<TAG:value>" to the frame (2 decimals, same format as before).
void addTag(String& frame, const char* tag, float value)
{
  frame += '<';
  frame += tag;
  frame += ':';
  frame += String(value, 2);
  frame += '>';
}

// Rejoin the ESP32's WiFi if the link dropped (e.g. the ESP32 rebooted).
void ensureWiFi()
{
  if (WiFi.status() == WL_CONNECTED) return;

  static uint32_t lastTry = 0;
  if (lastTry != 0 && millis() - lastTry < WIFI_RETRY_MS) return;
  lastTry = millis();

  client.stop();
  Serial.println("WiFi down - reconnecting...");
  WiFi.begin(WIFI_SSID, WIFI_PASS);
}

// Keep the TCP connection to the Elecrow alive.
void ensureTcp()
{
  if (client.connected()) return;

  static uint32_t lastTry = 0;
  if (lastTry != 0 && millis() - lastTry < TCP_RETRY_MS) return;
  lastTry = millis();

  client.stop();   // clear any dead socket
  Serial.println("Connecting to Elecrow TCP server...");
  if (client.connect(SERVER_IP, SERVER_PORT)) {
    Serial.println("Server connected!");
  } else {
    Serial.println("Connection failed, will retry...");
  }
}

void setup()
{
  Serial.begin(115200);
  Wire.begin();   // I2C controller

  Serial.println("Uno R4 booted. Joining Elecrow WiFi...");

  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("ERROR: WiFi module not found");
  }

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < WIFI_SETUP_WAIT_MS) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected!");
    Serial.print("Uno R4 IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nWiFi not connected yet - loop() will keep retrying.");
  }
}

void loop()
{
  ensureWiFi();
  if (WiFi.status() == WL_CONNECTED) ensureTcp();

  static uint32_t lastPoll = 0;
  if (millis() - lastPoll < POLL_INTERVAL_MS) return;
  lastPoll = millis();

  // ---- Poll sensors. A failed read is NOT sent (no fake 32 F). ----
  float n1[1], n2[2];
  bool t1_ok = readFloats(ADDR_NANO1, n1, 1);
  bool n2_ok = readFloats(ADDR_NANO2, n2, 2);   // T2 and FC come together

  float t1_F = t1_ok ? toF(n1[0]) : 0.0f;
  float t2_F = n2_ok ? toF(n2[0]) : 0.0f;
  float fc_F = n2_ok ? toF(n2[1]) : 0.0f;

#if USE_DEMO_SPEED
  demoScrewSpeed += 1.0f;
  if (demoScrewSpeed > 60.0f) demoScrewSpeed = 0.0f;
#endif

  // ---- Build one frame containing only valid readings. ----
  String frame;
  frame.reserve(96);
#if USE_DEMO_SPEED
  addTag(frame, "S_EXT", demoScrewSpeed);
#endif
  if (t1_ok) addTag(frame, "T_EX1", t1_F);
  if (n2_ok) {
    addTag(frame, "T_EX2", t2_F);
    addTag(frame, "T_FC",  fc_F);
  }

  // ---- Sendin a single write. ----
  bool sent = false;
  if (client.connected() && frame.length() > 0) {
    size_t n = client.write((const uint8_t*)frame.c_str(), frame.length());
    sent = (n == frame.length());
  }

  // ---- Debug msgs ----
  Serial.print(sent ? "Sent -> " : "NOT SENT -> ");
  Serial.print("T1: ");  if (t1_ok) Serial.print(t1_F); else Serial.print("---");
  Serial.print(" | T2: "); if (n2_ok) Serial.print(t2_F); else Serial.print("---");
  Serial.print(" | FC: "); if (n2_ok) Serial.print(fc_F); else Serial.print("---");
  Serial.print(" | frame: ");
  Serial.println(frame);
}