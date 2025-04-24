#include <WiFi.h>
#include <AsyncTCP.h>
#include <HTTPClient.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>
#include <DHT.h>
#include <ArduinoJson.h>
// WLAN-Zugangsdaten
//char* ssid = "LABOR-FRITZ-157";
//char* password = "2451368487361184";
//LABOR-FRITZ-157
//                  2451368487361184

// Server-URL
//const char* serverUrl = "http://192.168.188.50:8080/api/data";
String serverUrl = "http://192.168.188.50:8080/api/data";

// DHT-Sensor Setup
#define DHTPIN 4          // Pin, an dem der DHT angeschlossen ist
#define DHTTYPE DHT11     // DHT11 oder DHT22
DHT dht(DHTPIN, DHTTYPE);

AsyncWebServer server(80);
Preferences preferences;

const char* apSSID = "ESP_Config";
const char* apPassword = "12345678";

void startAPMode() {
  WiFi.softAP(apSSID, apPassword);
  Serial.println("Access Point gestartet: " + WiFi.softAPIP().toString());

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/html", R"rawliteral(
      <form action="/save" method="post">
        SSID: <input name="ssid"><br>
        Passwort: <input name="password" type="password"><br>
        Server: <input name="server"><br>
        <input type="submit" value="Speichern">
      </form>
    )rawliteral");
  });

  server.on("/save", HTTP_POST, [](AsyncWebServerRequest *request){
    if (request->hasParam("ssid", true) && request->hasParam("password", true) && request->hasParam("server", true)) {
      String newSSID = request->getParam("ssid", true)->value();
      String newPass = request->getParam("password", true)->value();
      String newServer = request->getParam("server", true)->value();

      preferences.begin("wifi", false);
      preferences.putString("ssid", newSSID);
      preferences.putString("password", newPass);
      preferences.putString("server", newServer);
      preferences.end();
      request->send(200, "text/html", "Daten gespeichert. Neustart...");
      delay(2000);
      ESP.restart();
    } else {
      request->send(400, "text/plain", "Fehlerhafte Daten");
    }
  });

  server.begin();
}

bool connectToWiFi() {
  preferences.begin("wifi", true);
  String storedSsid = preferences.getString("ssid", "");
  String storedPassword = preferences.getString("password", "");
  serverUrl = preferences.getString("server", "");

  preferences.end();

  if (storedSsid.isEmpty()) {
    Serial.println("Keine SSID gespeichert!");
    return false;
  }

  Serial.println("Verbinde mit WLAN: " + storedSsid);
  //Serial.println("Verbinde mit WLAN: " + storedPassword);

  WiFi.mode(WIFI_AP_STA);
  WiFi.disconnect();
  delay(100);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  WiFi.begin(storedSsid.c_str(), storedPassword.c_str());

  unsigned long startAttempt = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 30000) {
    delay(1000);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nVerbunden! IP: " + WiFi.localIP().toString());
    return true;
  } else {
    Serial.println("\nVerbindung fehlgeschlagen.");
    return false;
  }
}


// Messintervall (Millisekunden)
const unsigned long interval = 10000;
unsigned long previousMillis = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);
  dht.begin();

  if (!connectToWiFi()) {
    Serial.println("Keine Verbindung möglich. Starte AP-Modus.");
    startAPMode();
    return;
  }

  Serial.println("Verbunden mit IP: " + WiFi.localIP().toString());
}

void loop() {
  
  if (WiFi.status() != WL_CONNECTED) {
    // static unsigned long lastAttempt = 0;
    // if (millis() - lastAttempt > 10000) {
    //   Serial.println("Wiederhole Verbindungsversuch...");
    //   connectToWiFi();
    //   //WiFi.begin(storedSsid.c_str(), storedPassword.c_str());  // nochmaliger Versuch
    //   lastAttempt = millis();
    // }
    return;
  }

  if (WiFi.status() == WL_CONNECTED && WiFi.getMode() != WIFI_STA) {
    Serial.println("STA-Verbindung erfolgreich – deaktiviere AP");
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_STA);
  }

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();

    if (isnan(temperature) || isnan(humidity)) {
      Serial.println("Fehler beim Lesen des DHT!");
      return;
    }

    if (!serverUrl.isEmpty())
    {
      Serial.print("DB-Server: ");
      Serial.println(serverUrl);
      StaticJsonDocument<200> doc;
      doc["temperature"] = temperature;
      doc["humidity"] = humidity;

      String jsonString;
      serializeJson(doc, jsonString);
      Serial.println(jsonString);

      HTTPClient http;
      http.begin(serverUrl.c_str());
      http.addHeader("Content-Type", "application/json");
      int httpResponseCode = http.POST(jsonString);

      Serial.print("Antwort: ");
      Serial.println(httpResponseCode);

      if (httpResponseCode > 0) {
        Serial.println(http.getString());
      }

      http.end();
    }
  }

  delay(100);
}