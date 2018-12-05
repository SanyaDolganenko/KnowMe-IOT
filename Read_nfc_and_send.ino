
#include "SPI.h"
#include "PN532_SPI.h"
#include "snep.h"

#include "ESP8266WiFi.h"
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include "NdefMessage.h"


#include <Adafruit_NeoPixel.h>
# define LED D8
# define OUT D2

PN532_SPI pn532spi(SPI, 5);
SNEP nfc(pn532spi);
uint8_t ndefBuf[128];

const String eventId = "5bdc3bc1bc9fa03493e7d382";
//const char* server_login = "service01@gmail.com";
//const char* server_password = "service_device_01";

// WiFi parameters to be configured
const char* ssid = "Quartet";
//const char* ssid = "Oleksandr";
const char* password = "EQPACJDFP8";
//const char* password = "Dolganenko";
Adafruit_NeoPixel strip = Adafruit_NeoPixel(1, LED, NEO_GRB + NEO_KHZ800);
const int COLOR_NONE = 0;
const int COLOR_ORANGE = 1;
const int COLOR_RED = 2;
const int COLOR_GREEN = 3;

bool processingNFC = false;
//char* token = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpZCI6IjVjMDdhZTNiOWExY2I5MDAwNDcxMWQwOCIsImlhdCI6MTU0NDAzMTAyMX0.XOSx7dSdqJRNw6Dq_NughenvEbZAbEOSyineE3A9XP4"; //Length is 148
char* token = "";
HTTPClient http;

void setup(void) {
  pinMode(OUT, OUTPUT);
  strip.begin();
  Serial.begin(115200);
  Serial.println("Initializing device...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  //print a new line, then print WiFi connected and the IP address
  Serial.println("");
  Serial.println("WiFi connected");
  // Print the IP address
  Serial.println(WiFi.localIP());

  setColorOfIndicator(0);
  login();
}

void login() {
  http.begin("http://knowme-server.herokuapp.com/auth/login?lang=en");      //Specify request destination
  http.addHeader("Content-Type", "application/json");  //Specify content-type header
  int httpCode = http.POST("{ \"email\":\"service01@gmail.com\",\"password\":\"service_device_01\"}"); //Send the request
  String payload = http.getString();                  //Get the response payload
  http.writeToStream(&Serial);
  Serial.println(httpCode);   //Print HTTP return code
  Serial.println(payload);    //Print request response payload
  if (httpCode = 200) {
    const size_t bufferSize = JSON_ARRAY_SIZE(0) + JSON_OBJECT_SIZE(4) + JSON_OBJECT_SIZE(8) + JSON_OBJECT_SIZE(9) + 520;
    DynamicJsonBuffer jsonBuffer(bufferSize);
    JsonObject& root = jsonBuffer.parseObject(payload);
    if (!root.success()) {
      Serial.println("parseObject() failed");
      setColorOfIndicator(2);
    } else {
      if (strlen(token) == 0) {
        const char* tokenConst = root["token"];
        int lengthOfToken = strlen(tokenConst) + 1;
        token  = (char *)malloc(lengthOfToken);
        strcpy( token, tokenConst);
      }
      Serial.println(token);
    }
  } else {
    setColorOfIndicator(2);
  }
  http.end();
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    if (!processingNFC) {
      setColorOfIndicator(1);
      getMsgFromAndroid();
    }
  } else {
    setColorOfIndicator(2);
  }
  delay(3000);
}

void setColorOfIndicator(int color) {
  switch (color) {
    case COLOR_NONE:
      Serial.println("Setting color to blank");
      strip.setPixelColor(0, 0, 0, 0);
      strip.show();
      break;
    case COLOR_ORANGE:
      Serial.println("Setting color to orange");
      strip.setPixelColor(0, 255, 165, 0);
      strip.show();
      break;
    case COLOR_GREEN:
      Serial.println("Setting color to orange");
      strip.setPixelColor(0, 0, 255, 0);
      strip.show();
      break;
    case COLOR_RED:
      Serial.println("Setting color to orange");
      strip.setPixelColor(0, 255, 0, 0);
      strip.show();
      break;
  }
}


//void SendMsgToAndroid() {
//  Serial.println("Device is listening fot NFC peer.");
//  NdefMessage message = NdefMessage();
//  String sendAndroid;
//  sendAndroid = "RLED:" + String(digitalRead(RLED));
//  sendAndroid += ", GLED:" + String(digitalRead(GLED));
//  message.addTextRecord(sendAndroid);
//  //message.addUriRecord("http://shop.oreilly.com/product/mobile/0636920021193.do");
//  //message.addUriRecord("http://arduino.cc");
//  //message.addUriRecord("https://github.com/don/NDEF");
//
//
//  int messageSize = message.getEncodedSize();
//  if (messageSize > sizeof(ndefBuf)) {
//    Serial.println("ndefBuf is too small");
//    while (1) {
//    }
//  }
//  message.encode(ndefBuf);
//  if (0 >= nfc.write(ndefBuf, messageSize)) {
//    Serial.println("Failed");
//  } else {
//    Serial.println("Success");
//  }
//  //  Serial.println(digitalRead(SW));
//}



void getMsgFromAndroid() {
  Serial.println("Waiting for message from Peer");
  int msgSize = nfc.read(ndefBuf, sizeof(ndefBuf));
  if (msgSize > 0) {
    NdefMessage msg  = NdefMessage(ndefBuf, msgSize);
    msg.print();
    int recordCount = msg.getRecordCount();
    NdefRecord record = msg.getRecord(0);  //read 1 record
    String message = readMsg(record);
    Serial.println(message);
    checkUserOnServer(message);
    setColorOfIndicator(3);
  } else {
    Serial.println("Message is empty");
  }
}


String readMsg( NdefRecord record ) {
  int payloadLength = record.getPayloadLength();
  byte payload[payloadLength];
  record.getPayload(payload);
  String payloadAsString = "";
  for (int c = 0; c < payloadLength; c++) {
    payloadAsString += (char)payload[c];
  }
  return payloadAsString;
}

void checkUserOnServer(String userId) {
  Serial.println("Sending userId to server:");
  Serial.println(userId);
  Serial.println("Using token:");
  Serial.println(token);
  http.begin("http://knowme-server.herokuapp.com/service/check-user-for-event");      //Specify request destination
  http.addHeader("Content-Type", "application/json");  //Specify content-type header
  http.addHeader("token", token);
  String startPost = "{\"userId\":\"";
  String middlePost = "\", \"eventId\":\"";
  String endPost = "\"}";
  String request = startPost + userId + middlePost + eventId + endPost;
  Serial.println("Printing request:");
  Serial.println(request);
  int httpCode = http.POST(request);   //Send the request
  String payload = http.getString();                  //Get the response payload
  http.writeToStream(&Serial);
  Serial.println(httpCode);   //Print HTTP return code
  Serial.println("Got response");
  Serial.println(payload);    //Print request response payload

  http.end();  //Close connection
}
