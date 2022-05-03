#include <Servo.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Arduino.h>
#include <U8g2lib.h>
#include "FastLED.h"
#include <DHT.h>

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

// https://github.com/knolleary/pubsubclient

/************************* WiFi & Config *********************************/

#define WLAN_SSID         "PickMe"
#define WLAN_PASS         "fallout312345"
//const char* mqtt_server   = "broker.mqtt-dashboard.com";
const char* mqtt_server   = "mqtt.rniemand.com";
String CLIENT_NAME        = "esp8266";
#define NUM_LEDS          1
#define DATA_PIN          D2
#define DHTPIN            D3
#define DHTTYPE           DHT11
bool dhtReadSuccess       = false;
float humidity            = 0;
float temperature         = 0;
float heatIndex           = 0;
// rnInTopic  |  rnOutTopic

/************************* WiFi & Config *********************************/

U8G2_SSD1306_128X32_UNIVISION_F_SW_I2C u8g2(U8G2_R0, /* clock=*/ D5, /* data=*/ D6, /* reset=*/ U8X8_PIN_NONE);
WiFiClient espClient;
PubSubClient client(espClient);
CRGB leds[NUM_LEDS];
Servo myservo;
DHT                     dht(DHTPIN, DHTTYPE);

long lastMsg = 0;
long lastDhtPublish = 0;
char msg[50];
String line1 = "";
String line2 = "";
bool newMessage = false;


void setup(void) {
  Serial.begin(9600);
  Serial.println("Booting up");
  myservo.attach(D1);

  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);
  setLedColor(255, 0, 0);
  
  u8g2.begin();
  waitForWiFiConnection();
  connectMqtt();

  waitForDhtReady();
  //myservo.write(45);

  Serial.println("booted");
}

void loop(void) {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  long now = millis();

  if(newMessage == true) {
    updateScreen();
    newMessage = false;
    delay(500);
  } else {
    if (now - lastMsg > 5000) {
      lastMsg = now;

      int length = CLIENT_NAME.length() + 1;
      char tmp[length];
      CLIENT_NAME.toCharArray(tmp, length);
      client.publish("devices/heartbeat", tmp);
    }
  }

  if (now - lastDhtPublish > 10000) {
    readDhtValues();

    char _temp[10] = {0};
    dtostrf (temperature, 6 , 2, _temp);
    client.publish("home/lounge/temperature", _temp);
    Serial.print("temperature: ");
    Serial.println(_temp);

    char _humid[10] = {0};
    dtostrf (humidity, 6 , 2, _humid);
    client.publish("home/lounge/humidity", _humid);
    Serial.print("humidity: ");
    Serial.println(_humid);

    char _heat[10] = {0};
    dtostrf (heatIndex, 6 , 2, _heat);
    client.publish("home/lounge/heatIndex", _heat);
    Serial.print("heatIndex: ");
    Serial.println(_heat);

    Serial.println("publishing temperature data");
    lastDhtPublish = now;
  }
  
  //updateScreen();
  //delay(250);  
}

void connectMqtt() {
  Serial.print("Connecting to MQTT :");
  Serial.println(mqtt_server);
  print("Connecting MQTT", mqtt_server);
  
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  Serial.println("Connected to MQTT");
}

void callback(char* topic, byte* payload, unsigned int length) {
  //line1 = String("Message [" + topic + "]");
  line1 = topic;
  char tmp[length + 1];

  Serial.println(topic);
  Serial.println(length);
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    tmp[i] = (char)payload[i];
    newMessage = true;
  }
  tmp[length] = '\0';

  Serial.println();
  Serial.println(tmp);
  line2 = String(tmp);
  
  updateScreen();

  if(line2.indexOf("RED") != -1){
    setLedColor(255, 0, 0);
  }
  if(line2.indexOf("GREEN") != -1){
    setLedColor(0, 255, 0);
  }
  if(line2.indexOf("BLUE") != -1){
    setLedColor(0, 0, 255);
  }
  if(line2.indexOf("RGB,") == 0){
    Serial.println("------------------------");
    int posComma = line2.indexOf(',', 4);
    Serial.println(posComma);
    Serial.println(line2.indexOf(',', posComma));
    String _red = line2.substring(4, line2.indexOf(',', posComma));
    Serial.println(_red.toInt());
    posComma = line2.indexOf(',', posComma) + 1;

    Serial.println(posComma);
    Serial.println(line2.indexOf(',', posComma));
    String _green = line2.substring(posComma, line2.indexOf(',', posComma));
    Serial.println(_green.toInt());
    posComma = line2.indexOf(',', posComma) + 1;

    Serial.println(posComma);
    Serial.println(line2.indexOf(',', posComma));
    String _blue = line2.substring(posComma, line2.indexOf(',', posComma));
    Serial.println(_blue.toInt());

    setLedColor(_red.toInt(), _green.toInt(), _blue.toInt());
  }
  if(line2.indexOf("SERVO,") == 0){
    String _pos = line2.substring(6, line2.length());
    Serial.println(_pos.toInt());
    myservo.write(_pos.toInt());
  }

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is acive low on the ESP-01)
  } else {
    digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
  }

}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    print("Attempting MQTT connection...", "");
    // Create a random client ID
    //String clientId = "ESP8266Client-";
    //clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(CLIENT_NAME.c_str(), "esp8266", "bob")) {
      print("MQTT connected", "");
      // Once connected, publish an announcement...
      client.publish("outTopic", "hello world");
      // ... and resubscribe
      client.subscribe("rnInTopic");
    } else {
      print("failed, rc=" + client.state(), "Waiting 5 secs.");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void updateScreen() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_courR08_tf);
  u8g2.drawStr(0,8,"IP: ");
  u8g2.drawStr(20,8, getIpAddress());
  //u8g2.drawStr(0,16,"AP: " + WLAN_SSID);
  u8g2.drawStr(0,16, "AP: ");
  u8g2.drawStr(20,16, WLAN_SSID);

  int line1length = line1.length() + 1;
  char char_array_1[line1length];
  line1.toCharArray(char_array_1, line1length);
  u8g2.drawStr(0,24, char_array_1);
  
  int line2length = line2.length() + 1;
  char char_array_2[line2length];
  line2.toCharArray(char_array_2, line2length);
  u8g2.drawStr(0,32, char_array_2);
  
  u8g2.sendBuffer();
}

void waitForWiFiConnection() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_courR08_tf);
  u8g2.drawStr(0,8,"Connecting to");
  u8g2.drawStr(0,16, WLAN_SSID);
  u8g2.sendBuffer();
  char dots[20] = "";
  int pos = 0;

  Serial.println("Connecting to WiFi");
  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    dots[pos] = '.';
    u8g2.drawStr(0,24, dots);
    u8g2.sendBuffer();
    delay(500);
    pos++;
  }
  Serial.println();
  Serial.println("Connected");

  u8g2.drawStr(0,32,"Connected!");
  u8g2.sendBuffer();
  delay(500);

  line1 = "WiFi: Connected";
  line2 = "Testing...";
}

String ipToString(IPAddress ip){
  String s="";
  for (int i=0; i<4; i++)
    s += i  ? "." + String(ip[i]) : String(ip[i]);
  return s;
}

void print(String l1, String l2) {
  line1 = l1;
  line2 = l2;

  updateScreen();
}

char* getIpAddress() {
  int str_len = 15;
  char char_array[str_len];
  ipToString(WiFi.localIP()).toCharArray(char_array, str_len);
  return char_array;
}

void setLedColor(byte red, byte green, byte blue) {
  leds[0] = CRGB(red, green, blue);
  FastLED.show();
}

void waitForDhtReady() {
  Serial.print("Waiting for DHT ");
  
  while(!dhtReadSuccess) {
    delay(250);
    Serial.print(".");
    readDhtValues();
  }
  
  Serial.print(" READY (temperature: ");
  Serial.print(temperature);
  Serial.print(" *C) (humidity: ");
  Serial.print(humidity);
  Serial.print(" %) (heatIndex: ");
  Serial.print(heatIndex);
  Serial.print(")");
  Serial.println();
}

void readDhtValues() {
  dhtReadSuccess = false;

  float h = dht.readHumidity();
  float t = dht.readTemperature();

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t)) {
    return;
  }

  temperature = t;
  humidity = h;

  dhtReadSuccess = true;
  heatIndex = dht.computeHeatIndex(temperature, humidity, false);
  Serial.println("Updated DHT values");
}




