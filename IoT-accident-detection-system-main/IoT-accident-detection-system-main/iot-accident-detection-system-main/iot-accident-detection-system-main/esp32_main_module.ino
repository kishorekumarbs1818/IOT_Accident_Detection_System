#include <WiFi.h>
#include <HTTPClient.h>
#include <TinyGPS++.h>
#include <HardwareSerial.h>
#include <ArduinoJson.h>

TinyGPSPlus gps;
HardwareSerial SerialGPS(1);

const char* ssid = "*********";
const char* password = "*************";

int canSend = 1;
int lastState = HIGH;  // the previous state from the input pin
int currentState;

#define BUTTON_PIN 13

void setup() {
  pinMode(15, INPUT);
  pinMode(26, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  Serial.begin(115200);
  SerialGPS.begin(9600, SERIAL_8N1, 16, 17);
  WiFi.begin(ssid, password);
  Serial.println("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  //Send an HTTP POST request on button press
  int value = digitalRead(15);
  if (WiFi.status() == WL_CONNECTED) {
    if (value == 0) {
      digitalWrite(26, HIGH);
      canSend = 1;
      Serial.println("Triggered");
      for (int t = 0; t < 9999; t++)
        for (int tt = 0; tt < 5999; tt++) {
          currentState = digitalRead(BUTTON_PIN);
          if (lastState == LOW && currentState == HIGH) {
            Serial.println("False Alarm!!!");
            canSend = 0;
            digitalWrite(26, LOW);
          }
          lastState = currentState;
        }

      if (canSend) {
        HTTPClient callHTTP, notificationHTTP, reverse_geoHTTP, espcamHTTP;

        String b1 = "0.00";
        String b2 = "0.00";

        while (SerialGPS.available() > 0) {
          gps.encode(SerialGPS.read());
        }

        Serial.print("LAT = ");
        Serial.println(gps.location.lat(), 6);
        Serial.print("LONG = ");
        Serial.println(gps.location.lng(), 6);
        Serial.print("ALT = ");
        Serial.println(gps.altitude.meters());
        Serial.print("Sats = ");
        Serial.println(gps.satellites.value());

        b1 = String(gps.location.lat(), 6);
        b2 = String(gps.location.lng(), 6);

        notificationHTTP.begin("https://maker.ifttt.com/trigger/accident3/with/key/************/?value1=" + b1 + "&value2=" + b2);  // IFTTT key goes here
        int notification_code = notificationHTTP.GET();
        int nominatim_code = 0;
        while (nominatim_code != 200) {
          reverse_geoHTTP.begin("https://nominatim.openstreetmap.org/reverse?lat=" + b1 + "&lon=" + b2 + "&format=json");
          nominatim_code = reverse_geoHTTP.GET();
          Serial.println(nominatim_code);
        }
        String payload = reverse_geoHTTP.getString();
        char json[999];
        payload.toCharArray(json, 999);
        reverse_geoHTTP.end();

        StaticJsonDocument<999> doc;
        DeserializationError error = deserializeJson(doc, json);

        if (error) {
          Serial.print(F("deserializeJson() failed: "));
          Serial.println(error.f_str());
          return;
        }

        String addr = doc["display_name"];
        Serial.println(addr);
        String address = "";

        for (int j = 0; j < 100; j++) {
          if (addr[j] == ',')
            break;
          address = address + addr[j];
        }

        Serial.println(address);

        callHTTP.begin("https://maker.ifttt.com/trigger/accident/with/key/*************/?value1=" + address);  // IFTTT key here
        int call_code = callHTTP.GET();
        notificationHTTP.end();
        callHTTP.end();

        espcamHTTP.begin("http://**********/test?lat=" + b1 + "&long=" + b2);  // IP address of CAM module
        int httpCode = espcamHTTP.GET();

        if (httpCode > 0) {
          String payload = espcamHTTP.getString();
          Serial.println(httpCode);
          Serial.println(payload);
        } else {
          Serial.println("Error on HTTP request");
        }
        espcamHTTP.end();
      }
    }
  } else {
    Serial.println("WiFi Disconnected");
    delay(5000);
  }
}