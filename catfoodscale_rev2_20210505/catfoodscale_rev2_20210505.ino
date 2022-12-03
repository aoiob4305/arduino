//
// FILE: catfoodscale.ino
// AUTHOR: aoiob4305@gmail.com
// VERSION: 0.0.2
//
// HISTORY:
// 0.0.0    2021-04-17 initial version
// 0.0.1    2021-04-19 mqtt 추가 및 측정 주기 추가
// 0.0.2    2021-05-05 상수 선언방식 변경 및 주기 상수 변경

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <FS.h>
#include "HX711.h"
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

#include "set.h"

ESP8266WebServer server(80);

WiFiClient client;
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT);
Adafruit_MQTT_Publish msg_weight = Adafruit_MQTT_Publish(&mqtt, "/catfoodscale/weight");

HX711 scale;

float weight_val = 0.0;
float weight_prev = 0.0;

unsigned long startMillis;
unsigned long currentMillis;

//esp8266의 D1, D2 핀 이용
uint8_t dataPin = D1;
uint8_t clockPin = D2;

String getContentType(String filename);
bool handleFileRead(String path);
void handleRoot();
void handleNotFound();
void tareScale();
void calScale();
float weightScale();
void getScaleData();
void setScale();
void powerUpScale();
void powerDownScale();
void MQTT_connect();

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(STASSID, STAPSK);
  Serial.println("");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(STASSID);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("catfoodscale")) {
    Serial.println("MDNS responder started");
  }

  SPIFFS.begin(); 

  scale.begin(dataPin, clockPin);

  server.on("/", handleRoot);
  server.on("/tare", tareScale);
  server.on("/cal", calScale);
  server.on("/weight", weightScale);
  server.on("/getscaledata", getScaleData);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTP server started");

  //mqtt 타이머용
  startMillis = millis();
  setScale();
}

void loop() {
  currentMillis = millis();
  if(currentMillis - startMillis >= INTERVAL) {
    //Serial.println("->interval time");
    weight_val = weightScale();
    startMillis = currentMillis;
    if(abs(weight_prev - weight_val) > WEIGHT_TOL) {
      MQTT_connect();
      msg_weight.publish(weight_val);
      weight_prev = weight_val;       
    }
  }
  server.handleClient();
  MDNS.update();
}

String getContentType(String filename){
  if(filename.endsWith(".htm")) return "text/html";
  else if(filename.endsWith(".html")) return "text/html";
  else if(filename.endsWith(".css")) return "text/css";
  else if(filename.endsWith(".js")) return "application/javascript";
  else if(filename.endsWith(".png")) return "image/png";
  else if(filename.endsWith(".gif")) return "image/gif";
  else if(filename.endsWith(".jpg")) return "image/jpeg";
  else if(filename.endsWith(".ico")) return "image/x-icon";
  else if(filename.endsWith(".xml")) return "text/xml";
  else if(filename.endsWith(".pdf")) return "application/x-pdf";
  else if(filename.endsWith(".zip")) return "application/x-zip";
  else if(filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}

bool handleFileRead(String path) { // send the right file to the client (if it exists)
  Serial.println("handleFileRead: " + path);
  if (path.endsWith("/")) path += "index.html";         // If a folder is requested, send the index file
  String contentType = getContentType(path);            // Get the MIME type
  if (SPIFFS.exists(path)) {                            // If the file exists
    File file = SPIFFS.open(path, "r");                 // Open it
    size_t sent = server.streamFile(file, contentType); // And send it to the client
    file.close();                                       // Then close the file again
    return true;
  }
  Serial.println("\tFile Not Found");
  return false;                                         // If the file doesn't exist, return false
}

void handleRoot() {
  if (!handleFileRead(server.uri())) {
    server.send(404, "text/plain", "404: Not Found");
  }
}

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }

  server.send(404, "text/plain", message);
}

void tareScale() {
  powerUpScale();
  scale.tare();
  powerDownScale();
  
  server.send(200, "text/html", "tare a scale");
}

void calScale() {
  powerUpScale();
  scale.callibrate_scale(410, 5);
  powerDownScale();
  
  server.send(200, "text/html", "cal a scale");
}

float weightScale() {
  char weight[10];
  float result = 0.0;

  powerUpScale();
  result = scale.get_units(10);
  powerDownScale();
  
  snprintf(weight, 10, "%5.2f", result);
  Serial.print("UNITS: ");
  Serial.println(weight);
  server.send(200, "text/plain", weight);

  return result;
}

void getScaleData() {
  char scaledata[40];
  snprintf(scaledata, 40, "tare: %5.2f, gain: %5.2f, scale: %5.2f, offset: %5.2f", scale.get_tare(), scale.get_gain(), scale.get_scale(), scale.get_offset());
  server.send(200, "text/plain", scaledata);
  Serial.print("gain val: ");
  Serial.println(scale.get_gain());
  Serial.print("scale val: ");
  Serial.println(scale.get_scale());
  Serial.print("offset val: ");
  Serial.println(scale.get_offset());
}

void setScale() {
  scale.set_gain(SET_GAIN);
  scale.set_scale(SET_SCALE);
  scale.set_offset(SET_OFFSET);
}

void powerUpScale() {
  scale.power_up();
  Serial.print("wait for scale");
  while(!scale.is_ready()) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();
}

void powerDownScale() {
  scale.power_down();
}

void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
       Serial.println(mqtt.connectErrorString(ret));
       Serial.println("Retrying MQTT connection in 5 seconds...");
       mqtt.disconnect();
       delay(5000);  // wait 5 seconds
       retries--;
       if (retries == 0) {
         // basically die and wait for WDT to reset me
         while (1);
       }
  }
  Serial.println("MQTT Connected!");
}

// END OF FILE
