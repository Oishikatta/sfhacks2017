#include <Arduino.h>
#include <ESP8266WiFi.h>
//#include <WiFiClient.h>
//#include <ESP8266WebServer.h>
#include "fauxmoESP.h"
#include <IRremoteESP8266.h>
#include "custom_wifi.h"

fauxmoESP fauxmo;

const char* ssid = WIFI_SSID;
const char* password = WIFI_PASS;

//ESP8266WebServer server(80);
int x = 0;

const int LED = 4;
const int irLED = 15;
const int recvLED = 12;
IRsend irsend(irLED);
IRrecv irrecv(recvLED);
decode_results results;
bool status = false;

void setup() {
  // put your setup code here, to run once:
  WiFi.begin(ssid, password);
  Serial.begin(115200);
  Serial.println("setup");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  irsend.begin();
  irrecv.enableIRIn();

  Serial.println("IR setup complete.");

  /*
  server.on("/", [](){
    x++;
    String webPage = String("<h1>") + x + String("</h1>");
    server.send(200, "text/html", webPage);
    Serial.println( ESP.getFreeHeap() );
  });
  
  server.begin();
  Serial.println("HTTP server started");
  */

  pinMode(LED, OUTPUT);
  digitalWrite(LED, HIGH);

  fauxmo.addDevice("device one");
  fauxmo.onMessage([](unsigned char device_id, const char * device_name, bool state) {
        Serial.printf("[MAIN] Device #%d (%s) state: %s\n", device_id, device_name, state ? "ON" : "OFF");
        if ( state ) {
          // Turn ON
          irsend.sendSAMSUNG(0xE0E040BF, 32);
        } else {
          // Turn OFF
          irsend.sendSAMSUNG(0xE0E040BF, 32);
        }
    });
  Serial.println("fauxmo device added.");
}

void loop() {
  // put your main code here, to run repeatedly:
  //static int x = 0;
  //char* y = new char;
  //Serial.println(itoa(x, y, 10));
  //x += 1;
  //delay(100);
  //server.handleClient();
  fauxmo.handle();

  if (irrecv.decode(&results)) {
    Serial.println(results.value, HEX);
    dump(&results);
    irrecv.resume(); // Receive the next value
  }
  digitalWrite(LED, status);
  status = !status;

  delay(10);
}

// function from http://web.archive.org/web/20150723164858/http://as3breeze.com/arduino-sending-samsung-ir-codes
void dump(decode_results *results) {
  int count = results->rawlen;
  if (results->decode_type == UNKNOWN) {
    Serial.print("Unknown encoding: ");
  } 
  else if (results->decode_type == NEC) {
    Serial.print("Decoded NEC: ");
  } 
  else if (results->decode_type == SONY) {
    Serial.print("Decoded SONY: ");
  } 
  else if (results->decode_type == RC5) {
    Serial.print("Decoded RC5: ");
  } 
  else if (results->decode_type == RC6) {
    Serial.print("Decoded RC6: ");
  }
  else if (results->decode_type == PANASONIC) {
    Serial.print("Decoded PANASONIC: ");
  }
  else if (results->decode_type == JVC) {
    Serial.print("Decoded JVC: ");
  }
  else if (results->decode_type == SAMSUNG) {
    Serial.print("Decoded SAMSUNG: ");
  }
  int val1 = results->value;
  Serial.print(val1, HEX);
  Serial.print(" (");
  int valbits = results->bits;
  Serial.print(valbits, DEC);
  Serial.println(" bits)");
  Serial.print("Raw (");
  Serial.print(count, DEC);
  Serial.print("): ");

  for (int i = 0; i < count; i++) {
    if ((i % 2) == 1) {
      int valen = results->rawbuf[i]*USECPERTICK;
      Serial.print(valen, DEC);

    } 
    else {
      int negvalen =-(int)results->rawbuf[i]*USECPERTICK;
      Serial.print(negvalen, DEC);
    }
    Serial.print(", ");
  }
  Serial.println("");
}
