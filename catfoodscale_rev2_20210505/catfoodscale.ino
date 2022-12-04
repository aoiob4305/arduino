//
// FILE: catfoodscale.ino
// AUTHOR: aoiob4305@gmail.com
// VERSION: 0.0.4
//
// HISTORY:
// 0.0.0    2021-04-17 initial version
// 0.0.1    2021-04-19 mqtt 추가 및 측정 주기 추가
// 0.0.2    2021-05-05 상수 선언방식 변경 및 주기 상수 변경
// 0.0.3    2022-12-03 스케일 두개 사용하도록 기능 추가
// 0.0.4    2022-12-04 편차를 비교해 무게측정하는 방식을 제거하고 요청에 의한 무게측정으로 변경

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include "HX711.h"
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

#include "set.h"

ESP8266WebServer server(80);

WiFiClient client;
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT);
Adafruit_MQTT_Publish msg_scale_1st = Adafruit_MQTT_Publish(&mqtt, SCALE_1ST_TOPIC);
Adafruit_MQTT_Publish msg_scale_2nd = Adafruit_MQTT_Publish(&mqtt, SCALE_2ND_TOPIC);

HX711 scale_1st;
HX711 scale_2nd;


void handleRoot();
void handleNotFound();
void handleGetWeightAll();  // 무게를 측정하고 mqtt msg 발송까지

float weightScale(HX711 *scale);
void getScaleData(HX711 *scale);
void setScale(HX711 *scale, int init_data_gain, double init_data_scale, int init_data_offset);
void powerUpScale(HX711 *scale);
void powerDownScale(HX711 *scale);
void tareScale(HX711 *scale);
void calScale(HX711 *scale);
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

  scale_1st.begin(SCALE_1ST_DATAPIN, SCALE_1ST_CLKPIN);
  scale_2nd.begin(SCALE_2ND_DATAPIN, SCALE_2ND_CLKPIN);

  server.on("/", handleRoot);
  server.on("/getweightall", handleGetWeightAll);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTP server started");

  //mqtt 타이머용
  setScale(&scale_1st, SCALE_1ST_GAIN, SCALE_1ST_SCALE, SCALE_1ST_OFFSET);
  setScale(&scale_2nd, SCALE_2ND_GAIN, SCALE_2ND_SCALE, SCALE_2ND_OFFSET);
}

void loop() {  
  server.handleClient();
}

void handleGetWeightAll() {
  MQTT_connect();
  msg_scale_1st.publish(weightScale(&scale_1st));
  msg_scale_2nd.publish(weightScale(&scale_2nd));
  Serial.println("publish all scale's weight");

  server.send(200, "text/plain", "published");
}

void setScale(HX711 *scale, int init_data_gain, double init_data_scale, int init_data_offset) {
  scale->set_gain(init_data_gain);
  scale->set_scale(init_data_scale);
  scale->set_offset(init_data_offset);
}

float weightScale(HX711 *scale) {
  char weight[10];
  float result = 0.0;

  powerUpScale(scale);
  result = scale->get_units(10);
  powerDownScale(scale);
  
  snprintf(weight, 10, "%5.2f", result);
  Serial.print("UNITS: ");
  Serial.println(weight);

  return result;
}

void powerUpScale(HX711 *scale) {
  scale->power_up();
  Serial.print("wait for scale");
  while(!scale->is_ready()) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.println("Power up a scale.");
}

void powerDownScale(HX711 *scale) {
  scale->power_down();
  Serial.println("Power down a scale.");
}

void getScaleData(HX711 *scale) {
  char scaledata[40];
  snprintf(scaledata, 40, "tare: %5.2f, gain: %5.2f, scale: %5.2f, offset: %5.2f", scale->get_tare(), scale->get_gain(), scale->get_scale(), scale->get_offset());
  server.send(200, "text/plain", scaledata);
  Serial.println(scaledata);
}

void tareScale(HX711 *scale) {
  powerUpScale(scale);
  scale->tare();
  powerDownScale(scale);
}

void calScale(HX711 *scale) {
  powerUpScale(scale);
  scale->callibrate_scale(410, 5);
  powerDownScale(scale);
}

void handleRoot() {
  String message = "";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  Serial.print(message);
  server.send(200, "text/plain", message);
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
