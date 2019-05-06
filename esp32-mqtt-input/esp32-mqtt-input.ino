/*
  esp32-mqtt-input

  This project reads 10 digital pins, you can check the pins on the webserver and mqtt.

  created 05 05 2019
  by Luiz H. Cassettari - ricaun@gmail.com
*/

#include <WiFi.h>
#include <WebServer.h>
#include <PubSubClient.h>

/* PINOS */

#define PINO_MAX 10
const int PINO[] = {16, 17, 18, 19, 21, 22, 23, 25, 26, 27};

/* WIFI */

const char *ssid = "";
const char *password = "";

/* MQTT */

const char* mqtt_server = "iot.eclipse.org";
const char* mqtt_user = "";
const char* mqtt_pass = "";

// MQTT initial topic
const char* mqtt_topic = "esp32";

WebServer server(80);

const char *script = "<script>function loop() {var resp = GET_NOW('values'); document.getElementById('values').innerHTML = resp; setTimeout('loop()', 1000);} function GET_NOW(get) { var xmlhttp; if (window.XMLHttpRequest) xmlhttp = new XMLHttpRequest(); else xmlhttp = new ActiveXObject('Microsoft.XMLHTTP'); xmlhttp.open('GET', get, false); xmlhttp.send(); return xmlhttp.responseText; }</script>";

void handleRoot()
{
  String str = "";
  str += "<html>";
  str += "<head>";
  str += "<title>ESP32</title>";
  str += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  str += script;
  str += "</head>";
  str += "<body onload='loop()'>";
  str += "<center>";
  str += "<div id='values'></div>";
  str += "</center>";
  str += "</body>";
  str += "</html>";
  server.send(200, "text/html", str);
}

void handleGetValues()
{
  String str = "";

  for (int i = 0; i < PINO_MAX; i++)
  {
    str += handlePino(PINO[i]);
  }
  server.send(200, "text/plain", str);
}

String handlePino(int pino) {
  String str = "";
  str += "<p>";
  str += mqtt_getNamePino(pino);
  str += " : ";
  if (!digitalRead(pino))
  {
    str += "ON";
  }
  else
  {
    str += "OFF";
  }
  str += "</p>";
  return str;
}

// MQTT

WiFiClient client;
PubSubClient mqtt(client);

void mqtt_loop()
{
  if (mqtt.connected()) {
    mqtt.loop();
    if (runEvery_pino(50)) {
      pinos_change();
    }
    return;
  }

  mqtt.setServer(mqtt_server, 1883);

  // Loop until we're reconnected
  while (!mqtt.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESPClient-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (mqtt.connect(clientId.c_str(), mqtt_user, mqtt_pass)) {
      Serial.println("connected");

      for (int i = 0; i < PINO_MAX; i++)
      {
        int pino = PINO[i];
        mqtt.publish(mqtt_getNamePino(pino).c_str(), String(!digitalRead(pino)).c_str());
        Serial.print("publish : ");
        Serial.println(mqtt_getNamePino(pino));
      }
    } else {
      Serial.print("mqtt failed, rc=");
      Serial.print(mqtt.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }

}

void pinos_change() {
  static bool pinos[PINO_MAX];
  for (int i = 0; i < PINO_MAX; i++)
  {
    int pino = PINO[i];
    bool pino_io = !digitalRead(pino);
    if (pinos[i] != pino_io)
    {
      pinos[i] = pino_io;
      mqtt.publish(mqtt_getNamePino(pino).c_str(), String(pino_io).c_str());
      Serial.print("change publish : ");
      Serial.println(mqtt_getNamePino(pino));
    }
  }
}

boolean runEvery_pino(unsigned long interval)
{
  static unsigned long previousMillis = 0;
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval)
  {
    previousMillis = currentMillis;
    return true;
  }
  return false;
}

String mqtt_getNamePino(int pino) {
  String str = "";
  str += mqtt_topic;
  str += "/";
  str += pino;
  return str;
}

// Setup

void setup(void)
{
  Serial.begin(115200);

  for (int i = 0; i < PINO_MAX; i++)
  {
    pinMode(PINO[i], INPUT_PULLUP);
  }

  WiFi.mode(WIFI_STA);
  if (ssid != "")
    WiFi.begin(ssid, password);
  WiFi.begin();
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/values", handleGetValues);
  server.begin();
  Serial.println("HTTP server started");

}

void loop(void)
{
  mqtt_loop();
  server.handleClient();
}
