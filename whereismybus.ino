// rev1
// 현재 오고있는 버스의 남은시간을 가져와 LED에 나타냄
// 사용모듈: NODEMCU 1.0 (ESP12-e)
// LED타입: WS2801 RBG 스트립
// 각줄의 첫번째 LED는 항상켜져 있도록 하고 시간을 LED하나당 1분으로 위치처럼 표시
// 붐비는 정도를 가져와서 표시함, 붐빌경우(2) led를 적색으로 표기

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <FastLED.h>

#define DEBUG true

// LED 제어 관련
#define NUM_LEDS 22
#define NUM_LEDS_ROW 11
#define DATA_PIN D5
#define CLOCK_PIN D7
#define MAX_POWER 20

//와이파이 관련 
#define SSID ""
#define PASS ""

//버스 데이터 관련
#define HOWMANYBUSES 10
#define TIME_TO_HURRY_UP 6
#define MY_BUS_NUM_1 "5602"
#define MY_BUS_NUM_2 "5604"
#define MY_BUS_CROWDED 2

typedef struct BUS {
  String num = "";
  int time1 = 0;
  int time2 = 0;  
  int crowded1 = 0;
  int crowded2 = 0;
};

void wifiInit();
void getXmlData();
String getXmlElementContent(int pos_from, const String element_text);
void getUpcomingBuses();
BUS whereIsMyBus(const String mybus_num);
void displayMyBusOnLedStrip(BUS bus, int row);
void printResultsToSerial();

// 버스 데이터 관련
int buses_count = 0;
BUS buses[HOWMANYBUSES];

String rawXML = "";

const String URL = "http://api.gbis.go.kr/ws/rest/busarrivalservice/station?serviceKey=1234567890&stationId=";
const String STATION_ID = "224000081";

//led 관련
CRGB leds[NUM_LEDS];

void setup() {
  Serial.begin(115200);
  Serial.println();

  wifiInit();

  FastLED.addLeds<WS2801, DATA_PIN, CLOCK_PIN, RBG>(leds, NUM_LEDS);
  FastLED.setMaxPowerInMilliWatts(MAX_POWER);
}

void loop() {
  getXmlData();
  getUpcomingBuses();
  
  if(DEBUG == true) printResultsToSerial();

  BUS mybus;
  mybus = whereIsMyBus(MY_BUS_NUM_1);
  displayMyBusOnLedStrip(mybus, 0);
  mybus = whereIsMyBus(MY_BUS_NUM_2);
  displayMyBusOnLedStrip(mybus, 1);
  
  delay(60000);
}

void wifiInit() {
  WiFi.begin(SSID, PASS);

  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  Serial.print("Connected, IP address: ");
  Serial.println(WiFi.localIP());
  Serial.printf("Connection status: %d\n", WiFi.status());
}

void getXmlData() {
  WiFiClient client;
  HTTPClient http;

  if (DEBUG == true) Serial.print("[HTTP] begin...\n");

  if (http.begin(client, URL + STATION_ID)) {
    if (DEBUG == true) Serial.print("[HTTP] GET...\n");
    int httpCode = http.GET();

    // httpCode will be negative on error
    if (httpCode > 0) {
      // HTTP header has been send and Server response header has been handled
      if (DEBUG == true) Serial.printf("[HTTP] GET... code: %d\n", httpCode);

      // file found at server
      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
         rawXML = http.getString();
       }
    } else {
      if (DEBUG == true) Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }
    http.end();
  } else {
    if (DEBUG == true) Serial.printf("[HTTP] Unable to connect\n");
  }
}

void getUpcomingBuses() {
  int pos = 0;
  int index = 0;

  while (true) {
    pos = rawXML.indexOf("<busArrivalList>", pos + 1);

    if(pos != -1) {
      buses[index] = {
        getXmlElementContent(pos, "routeName"),
        getXmlElementContent(pos, "predictTime1").toInt(),
        getXmlElementContent(pos, "predictTime2").toInt(),
        getXmlElementContent(pos, "crowded1").toInt(),
        getXmlElementContent(pos, "crowded2").toInt()
      };
      index++;
    } else {
      if (DEBUG == true) Serial.println("end of xml");
      buses_count = index;
      break;
    }
  }
}

String getXmlElementContent(int pos_from, const String element_text) {
  int pos_item_start = 0;
  int pos_item_end = 0;

  String content = "";

  String element_start = "<" + element_text + ">";
  String element_end = "</" + element_text + ">";

  pos_item_start = rawXML.indexOf(element_start, pos_from) + element_start.length() ;
  pos_item_end = rawXML.indexOf(element_end, pos_from);

  if((pos_item_start && pos_item_end) != -1) {
    for(int i=0; i < (pos_item_end - pos_item_start); i++) {
      content += rawXML.charAt(pos_item_start + i);
    }
  } else {
    if (DEBUG == true) Serial.println("xml data is wrong or there is not the element");
  }

  return content;
}

void printResultsToSerial() {
  Serial.print("Upcoming buses at the station: ");
  Serial.println(buses_count);
  
  for(int i=0; i < buses_count; i++) {
    Serial.println("----------------------------------");
    Serial.print("bus num: ");
    Serial.println(buses[i].num);
    Serial.print("1st Bus: ");
    Serial.println(buses[i].time1);
    Serial.print("is 1st Bus crowded: ");
    Serial.println(buses[i].crowded1);
    Serial.print("2nd Bus: ");
    Serial.println(buses[i].time2);
    Serial.print("is 2nd Bus crowded: ");
    Serial.println(buses[i].crowded2);
    Serial.println("----------------------------------");
  }
}

BUS whereIsMyBus(const String mybus_num) {
  BUS mybus;
  for(int i=0; i < buses_count; i++) {
    if(buses[i].num == mybus_num) {
      mybus = buses[i];
      break; 
    }
  }
  return mybus;
}

void displayMyBusOnLedStrip(BUS bus, int row) {
  if(row == 0) {
    for(int i=0; i<NUM_LEDS_ROW; i++){
      leds[i] = CRGB::Black;
    }
    //각 줄의 첫번째 LED를 하얀색으로 점등하기 위해서
    leds[0] = CRGB::White;
    FastLED.show();
    delay(500);

    if ((bus.time1 > 0) && (bus.time1 < NUM_LEDS_ROW)) {
      if (bus.crowded1 == MY_BUS_CROWDED) {
        leds[bus.time1] = CRGB::Red;
      } else {
        leds[bus.time1] = CRGB::Green;
      }
    }

    if ((bus.time2 > 0) && (bus.time2 < NUM_LEDS_ROW)) {
      if (bus.crowded2 == MY_BUS_CROWDED) {
        leds[bus.time2] = CRGB::Red;
      } else {
        leds[bus.time2] = CRGB::Green;
      }
    }
  } else {
    for(int i=NUM_LEDS_ROW; i<NUM_LEDS; i++){
      leds[i] = CRGB::Black;
    }
    //각 줄의 첫번째 LED를 하얀색으로 점등하기 위해서
    leds[NUM_LEDS-1] = CRGB::White;
    FastLED.show();
    delay(500);

    if ((bus.time1 > 0) && (bus.time1 < NUM_LEDS_ROW)) {
      if (bus.crowded1 == TIME_TO_HURRY_UP) {
        leds[NUM_LEDS - bus.time1 - 1] = CRGB::Red;
      } else {
        leds[NUM_LEDS - bus.time1 - 1] = CRGB::Green;
      }
    }

    if ((bus.time2 > 0) && (bus.time2 < NUM_LEDS_ROW)) {
      if (bus.time2 < TIME_TO_HURRY_UP) {
        leds[NUM_LEDS - bus.time2 - 1] = CRGB::Red;
      } else {
        leds[NUM_LEDS - bus.time2 - 1] = CRGB::Green;
      }
    }    
  }
  FastLED.show();
}
