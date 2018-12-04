#include "SPI.h"
#include "PN532_SPI.h"
#include "snep.h"

#include "ESP8266WiFi.h"
#include <ESP8266HTTPClient.h>
#include "NdefMessage.h"

#include <Adafruit_NeoPixel.h>
# define LED D8
# define OUT D2

PN532_SPI pn532spi(SPI, 5);
SNEP nfc(pn532spi);
uint8_t ndefBuf[128];

// WiFi parameters to be configured
const char* ssid = "Quartet";
const char* password = "EQPACJDFP8";
Adafruit_NeoPixel strip = Adafruit_NeoPixel(1, LED, NEO_GRB + NEO_KHZ800);
const int COLOR_NONE = 0;
const int COLOR_ORANGE = 1;
const int COLOR_RED = 2;
const int COLOR_GREEN = 3;

bool processingNFC = false;

String readMsg( NdefRecord record ) {
  int payloadLength = record.getPayloadLength();
  byte payload[payloadLength];
  record.getPayload(payload);
  String payloadAsString = "";
  for (int c = 0; c < payloadLength; c++) {
    payloadAsString += (char)payload[c];
  }
  return payloadAsString.substring(3);
}

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
  // Connect to WiFi

}

void loop() {
  if (WiFi.status() == WL_CONNECTED) { //Check WiFi connection status
    if (!processingNFC) {
      setColorOfIndicator(1);
      getMsgFromAndroid();
      delay(3000);
    }
    //    HTTPClient http;    //Declare object of class HTTPClient
    //
    //    http.begin("http://knowme-server.herokuapp.com/auth/login?lang=en");      //Specify request destination
    //    http.addHeader("Content-Type", "application/json");  //Specify content-type header
    //
    //    int httpCode = http.POST("{ \"email\":\"kergudu@gmail.com\",\"password\":\"test1\"}");   //Send the request
    //    String payload = http.getString();                  //Get the response payload
    //    http.writeToStream(&Serial);
    //    Serial.println(httpCode);   //Print HTTP return code
    //    Serial.println(payload);    //Print request response payload
    //
    //    http.end();  //Close connection
  } else {
    setColorOfIndicator(2);
  }
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
    Serial.println("Cleared message:");
    Serial.println(message);
    setColorOfIndicator(3);
  } else {
    Serial.println("Failed");
  }
}
