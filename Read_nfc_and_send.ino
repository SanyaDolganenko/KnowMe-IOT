
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

const char* server_login = "service01@gmail.com";
const char* server_password = "service_device_01";

// WiFi parameters to be configured
//const char* ssid = "Quartet";
//const char* password = "EQPACJDFP8";
//String ssid = "Oleksandr";
//String password = "Dolganenko";
const int TIMEOUT_DURATION_SEC = 5;
//const char* ssid = "nure_5G";
//const char* password = "";
Adafruit_NeoPixel strip = Adafruit_NeoPixel(1, LED, NEO_GRB + NEO_KHZ800);
const int COLOR_NONE = 0;
const int COLOR_ORANGE = 1;
const int COLOR_RED = 2;
const int COLOR_GREEN = 3;

bool isInServiceMode = false;
bool processingNFC = false;
char* token = "";
String eventId;
HTTPClient http;

void setup(void) {
  pinMode(OUT, OUTPUT);
  strip.begin();
  Serial.begin(115200);
  Serial.println("Initializing device");
  connectAndLogin("Oleksandr", "Dolganenko");
}

void connectAndLogin(String ssid, String password) {
  WiFi.begin(ssid.c_str(), password.c_str());
  Serial.println("Connecting to WiFi...");
  int counter = 0;
  while (WiFi.status() != WL_CONNECTED && counter <= TIMEOUT_DURATION_SEC) {
    delay(1000);
    Serial.print(".");
    blink(COLOR_ORANGE);
    counter++;
  }
  Serial.println("");
  if (counter >= TIMEOUT_DURATION_SEC && WiFi.status() != WL_CONNECTED) {
    Serial.println("Unable to connect. Turning to service mode.");
    setColorOfIndicator(COLOR_ORANGE);
    isInServiceMode = true;
    while (WiFi.status() != WL_CONNECTED) {
      getMsgFromAndroid(true);
    }
  } else  {
    isInServiceMode = false;
    Serial.println("WiFi connected");
    // Print the IP address
    Serial.println(WiFi.localIP());
    setColorOfIndicator(0);
    login();
  }
}

void login() {
  Serial.println("Logging in...");
  http.begin("http://knowme-server.herokuapp.com/auth/login?lang=en");      //Specify request destination
  http.addHeader("Content-Type", "application/json");  //Specify content-type header
  const int capacity = JSON_OBJECT_SIZE(2);
  StaticJsonBuffer<capacity> jb;
  JsonObject& obj = jb.createObject();
  obj.set("email", server_login);
  obj.set("password", server_password);
  char request[150];
  obj.printTo(request);
  int httpCode = http.POST(request); //Send the request
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
      const char* eventIdConst = root["user"]["serviceEventId"];
      eventId = String(eventIdConst);
      Serial.println(eventId);
    }
  } else {
    setColorOfIndicator(COLOR_RED);
  }
  http.end();
}

void loop() {
  Serial.println("Main loop");
  if (!processingNFC) {
    getMsgFromAndroid(false);
  }
  if (WiFi.status() == WL_CONNECTED) {

  } else {
    setColorOfIndicator(COLOR_RED);
  }
  delay(2000);
}

void setColorOfIndicator(int color) {
  switch (color) {
    case COLOR_NONE:
      strip.setPixelColor(0, 0, 0, 0);
      strip.show();
      break;
    case COLOR_ORANGE:
      strip.setPixelColor(0, 255, 165, 0);
      strip.show();
      break;
    case COLOR_GREEN:
      Serial.println("Setting color to green");
      strip.setPixelColor(0, 0, 255, 0);
      strip.show();
      break;
    case COLOR_RED:
      Serial.println("Setting color to red");
      strip.setPixelColor(0, 255, 0, 0);
      strip.show();
      break;
  }
}

void blink(int color) {
  setColorOfIndicator(color);
  delay(300);
  setColorOfIndicator(0);
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



void getMsgFromAndroid(boolean serviceOnly) {
  Serial.println("Waiting for message from Peer");
  int msgSize = nfc.read(ndefBuf, sizeof(ndefBuf));
  if (msgSize > 0) {
    NdefMessage msg  = NdefMessage(ndefBuf, msgSize);
    msg.print();
    int recordCount = msg.getRecordCount();
    NdefRecord record = msg.getRecord(0);  //read 1 record
    String message = readMsg(record);
    Serial.println(message);
    String configs[2];

    if (message.indexOf("|") >= 0) {

      int r  = 0;
      int t = 0;
      for (int i = 0; i < message.length(); i++)
      {
        if (message.charAt(i) == '|' || i == message.length() - 1)
        {
          if (i == message.length() - 1) {
            i++;
          }
          if (t <= 2 ) {
            configs[t] = message.substring(r, i);
            r = (i + 1);
            t++;
          }
        }
      }
      String ssid = configs[0];
      String password = configs[1];
      Serial.println("Received config message");
      Serial.println(ssid);
      Serial.println(password);
      connectAndLogin(ssid, password);
    } else {
      if (serviceOnly) {
        Serial.println("Received service message is not in correct format.");
        blink(COLOR_RED);
        setColorOfIndicator(COLOR_ORANGE);
      } else {
        checkUserOnServer(message);
      }
    }
  } else {
    Serial.println("Message is empty");
    if (isInServiceMode) {
      blink(COLOR_RED);
      setColorOfIndicator(COLOR_ORANGE);
    }
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
  const int capacity = JSON_OBJECT_SIZE(2);
  StaticJsonBuffer<capacity> jb;
  JsonObject& obj = jb.createObject();
  obj.set("userId", userId.c_str());
  obj.set("eventId", eventId.c_str());
  char request[110];
  obj.printTo(request);
  Serial.println("Printing formed request:");
  Serial.println(request);
  int httpCode = http.POST(request);   //Send the request
  String payload = http.getString();                  //Get the response payload
  http.writeToStream(&Serial);
  Serial.println(httpCode);   //Print HTTP return code
  Serial.println("Got response");
  Serial.println(payload);    //Print request response payload
  if (httpCode == 200) {
    blink(COLOR_GREEN);
  } else {
    blink(COLOR_RED);
  }
  http.end();  //Close connection
}
