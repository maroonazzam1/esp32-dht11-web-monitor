#include <WiFi.h>
#include <WebServer.h>
#include "DHT.h"

// ---- EDIT THESE TWO LINES ----
const char* ssid     = "";
const char* password = "";
// ------------------------------

#define DHTPIN 4
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);
WebServer server(80);

float tempC = 0;
float humidity = 0;
unsigned long lastRead = 0;

void readSensor() {
  if (millis() - lastRead < 2000) return;   // DHT11 needs 2s between reads
  lastRead = millis();

  float h = dht.readHumidity();
  float t = dht.readTemperature();
  if (!isnan(h) && !isnan(t)) {
    humidity = h;
    tempC = t;
  }
}

void handleRoot() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta charset='utf-8'>";
  html += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
  html += "<meta http-equiv='refresh' content='5'>";   // auto-refresh every 5s
  html += "<title>ESP32 Room Monitor</title>";
  html += "<style>body{font-family:sans-serif;text-align:center;margin-top:60px;background:#111;color:#eee}";
  html += ".v{font-size:64px;font-weight:bold;margin:10px}.l{color:#888;font-size:14px;letter-spacing:2px}</style>";
  html += "</head><body>";
  html += "<div class='l'>TEMPERATURE</div><div class='v'>" + String(tempC, 1) + " &deg;C</div>";
  html += "<div class='l'>HUMIDITY</div><div class='v'>" + String(humidity, 0) + " %</div>";
  html += "</body></html>";

  server.send(200, "text/html", html);
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  dht.begin();

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.print("Connected. Open this address in your browser: http://");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.begin();
}

void loop() {
  server.handleClient();
  readSensor();
}