#define DEBUGGING true
#define DEBUG true

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
//#include <WiFiClient.h>
//#include <ESP8266WebServer.h>
#include "fauxmoESP.h"
#include <IRremoteESP8266.h>
#include <WebSocketClient.h>
#include <FS.h>
#include <Hash.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFSEditor.h>
#include "custom_wifi.h"

fauxmoESP fauxmo;
WebSocketClient webSocketClient;
// ?? Use WiFiClient class to create TCP connections
WiFiClient client;
//char path[] = "/";
//char host[] = "websocket.iotbridge.regex.be";
char path[] = "/socket.io/?EIO=3&transport=websocket";
char host[] = "necrosocket.herokuapp.com";

//#define WEBSOCKET_HOST "192.168.43.121"
#define WEBSOCKET_HOST "54.243.117.208"
//#define WEBSOCKET_PORT 9999
#define WEBSOCKET_PORT 4010

const char* ssid = WIFI_SSID;
const char* password = WIFI_PASS;
const char * hostName = "slpd";

//ESP8266WebServer server(80);
AsyncWebServer server(80);
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
  WiFi.hostname(hostName);
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


  ArduinoOTA.setHostname(hostName);
  ArduinoOTA.begin();

  MDNS.addService("http","tcp",80);

  SPIFFS.begin();

  server.on("/heap", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", String(ESP.getFreeHeap()));
  });

  server.onNotFound([](AsyncWebServerRequest *request){
    Serial.printf("NOT_FOUND: ");
    if(request->method() == HTTP_GET)
      Serial.printf("GET");
    else if(request->method() == HTTP_POST)
      Serial.printf("POST");
    else if(request->method() == HTTP_DELETE)
      Serial.printf("DELETE");
    else if(request->method() == HTTP_PUT)
      Serial.printf("PUT");
    else if(request->method() == HTTP_PATCH)
      Serial.printf("PATCH");
    else if(request->method() == HTTP_HEAD)
      Serial.printf("HEAD");
    else if(request->method() == HTTP_OPTIONS)
      Serial.printf("OPTIONS");
    else
      Serial.printf("UNKNOWN");
    Serial.printf(" http://%s%s\n", request->host().c_str(), request->url().c_str());

    if(request->contentLength()){
      Serial.printf("_CONTENT_TYPE: %s\n", request->contentType().c_str());
      Serial.printf("_CONTENT_LENGTH: %u\n", request->contentLength());
    }

    int headers = request->headers();
    int i;
    for(i=0;i<headers;i++){
      AsyncWebHeader* h = request->getHeader(i);
      Serial.printf("_HEADER[%s]: %s\n", h->name().c_str(), h->value().c_str());
    }

    int params = request->params();
    for(i=0;i<params;i++){
      AsyncWebParameter* p = request->getParam(i);
      if(p->isFile()){
        Serial.printf("_FILE[%s]: %s, size: %u\n", p->name().c_str(), p->value().c_str(), p->size());
      } else if(p->isPost()){
        Serial.printf("_POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
      } else {
        Serial.printf("_GET[%s]: %s\n", p->name().c_str(), p->value().c_str());
      }
    }

    request->send(404);
  });

  server.begin();
  connectToWebSocket();
}
unsigned long Timer = 0;
unsigned int attempts = 0;

void connectToWebSocket() {
  attempts++;
  if ( attempts > 2 ) {
    Serial.println("Too many reconnect attempts.");
    return;
  }
  // Connect to the websocket server
  if (client.connect(WEBSOCKET_HOST, WEBSOCKET_PORT)) {
    Serial.println("Connected");
  } else {
    Serial.println("Connection failed.");
  }

  // Handshake with the server
  webSocketClient.path = path;
  webSocketClient.host = host;
  if (webSocketClient.handshake(client)) {
    Serial.println("Handshake successful");
  } else {
    Serial.println("Handshake failed.");
  }
}



void loop() {
  String data;
  // put your main code here, to run repeatedly:
  //static int x = 0;
  //char* y = new char;
  //Serial.println(itoa(x, y, 10));
  //x += 1;
  //delay(100);
  //server.handleClient();
  fauxmo.handle();
  ArduinoOTA.handle();
  
  if (irrecv.decode(&results)) {
    Serial.println(results.value, HEX);
    dump(&results);
    irrecv.resume(); // Receive the next value
  }
  
  digitalWrite(LED, status);
  status = !status;

  if (client.connected()) {
    //data = String("rawr");
    //webSocketClient.sendData(data);
    webSocketClient.getData(data);
    if (data.length() > 0) {
      Serial.print("Received data: ");
      Serial.println(data);
    }
    
    //webSocketClient.sendData(data);
  } else if (Timer != 0 && millis() - Timer >= 15000) {
    Serial.println("Reconnecting websocket.");
    connectToWebSocket();
    Timer = 0;
  } else if (Timer == 0) {
    Serial.println("Client disconnected. Scheduling reconnect attempt.");
    Timer = millis();
  }
  
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
