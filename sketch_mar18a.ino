#include <Arduino.h>
#include <cstdlib>
#include <string>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>
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
//WebSocketClient webSocketClient;
// ?? Use WiFiClient class to create TCP connections
WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;

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
/*const IPAddress ip(192, 168, 43, 110);
const IPAddress dns(8,8,8,8);
const IPAddress gateway(192, 168, 43, 1);
const IPAddress subnet(255,255,255,0);*/
const char * hostName = "slpd";
const char* mqtt_server = "broker.mqtt-dashboard.com";

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

class SamsungRemote {
  public:
  static const int bits = 32;
  
  static const unsigned long btn0 = 0xE0E08877;
  static const unsigned long btn1 = 0xE0E020DF;
  static const unsigned long btn2 = 0xE0E0A05F;
  static const unsigned long btn3 = 0xE0E0609F;
  static const unsigned long btn4 = 0xE0E010EF;
  static const unsigned long btn5 = 0xE0E0906F;
  static const unsigned long btn6 = 0xE0E050AF;
  static const unsigned long btn7 = 0xE0E030CF;
  static const unsigned long btn8 = 0xE0E0B04F;
  static const unsigned long btn9 = 0xE0E0708F;

  static const unsigned long pwr_on = 0xE0E040BF;
  static const unsigned long pwr_off = 0xE0E040BF;

  static const unsigned long input = 0xE0E0807F;

  static const unsigned long ch_up = 0xE0E048B7;
  static const unsigned long ch_dn = 0xE0E008F7;

  static const unsigned long vol_up = 0xE0E0E01F;
  static const unsigned long vol_dn = 0xE0E0D02F;

  static const unsigned long mute = 0xE0E0F00F;
  static const unsigned long menu = 0xE0E058A7;
};

void setChannel(const char* channelNumber) {
  for ( int i = 0; channelNumber[i]; i++ ) {
    Serial.print("Sending: ");
    Serial.println( channelNumber[i] );

    unsigned long code;
    switch ( channelNumber[i] ) {
      case '0':
        code = SamsungRemote::btn0;
        break;
      case '1':
        code = SamsungRemote::btn1;
        break;
      case '2':
        code = SamsungRemote::btn2;
        break;
      case '3':
        code = SamsungRemote::btn3;
        break;
      case '4':
        code = SamsungRemote::btn4;
        break;
      case '5':
        code = SamsungRemote::btn5;
        break;
      case '6':
        code = SamsungRemote::btn6;
        break;
      case '7':
        code = SamsungRemote::btn7;
        break;
      case '8':
        code = SamsungRemote::btn8;
        break;
      case '9':
        code = SamsungRemote::btn9;
        break;
        
    }
    irsend.sendSAMSUNG(code, SamsungRemote::bits);
    delay(100);
  }
}

void sendCode(unsigned long code) {
  irsend.sendSAMSUNG(code, SamsungRemote::bits);
}

void loopSendCode(unsigned long code, int duration) {
  for ( int i = 0; i < duration; i++ ) {
    sendCode(code);
    delay(100);
  }
}

void setup() {
  // put your setup code here, to run once:
  WiFi.hostname(hostName);
  //WiFi.config(ip, gateway, subnet, dns);
  WiFi.begin(ssid, password);
  Serial.begin(115200);
  Serial.println("setup");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());
  
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  irsend.begin();
  irrecv.enableIRIn();

  Serial.println("IR setup complete.");

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

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void callback(char* topic, byte* payloadBytes, unsigned int length) {
  char* payload = new char[length];
  for ( int i = 0; i < length; i++ ) {
    payload[i] = (char)payloadBytes[i];
  }
  payload[length] = '\0';
  
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  Serial.println(payload);

  const char* channelCmd = "channel ";
  const int channelCmdLength = 8;
  const char* volumeCmd = "volume ";
  const int volumeCmdLength = 7;
  const char* menuCmd = "menu function: ";
  const int menuCmdLength = 15;
  
  if ( strncmp(payload, channelCmd, 8) == 0 ) {
    //const char* channelNumber = &payload[channelCmdLength];
    //int channelNumber = atoi(&payload[channelCmdLength]);
    setChannel(&payload[channelCmdLength]);
    // Serial.println( channelNumber );
  } else if ( strncmp(payload, volumeCmd, volumeCmdLength) == 0 ) {
    const char* volumeC2 = &payload[volumeCmdLength];
    Serial.println(volumeC2[0]);
    if ( volumeC2[0] == '+' ) {
      loopSendCode(SamsungRemote::vol_up, atoi((volumeC2++))*.4);
    } else if ( volumeC2[0] == '-' ) {
      Serial.println("Sending vol_dn");
      loopSendCode(SamsungRemote::vol_up, atoi((volumeC2++))*.4);
    } else if ( volumeC2[0] == 'u' ) {
      sendCode(SamsungRemote::vol_up);
    } else if ( volumeC2[0] == 'd' ) {
      sendCode(SamsungRemote::vol_dn);
    }
    int volumeNumber = atoi(&payload[volumeCmdLength]);
  } else if ( strncmp(payload, menuCmd, menuCmdLength) == 0 ) {
    const char* menuC2 = &payload[menuCmdLength];
    if ( strncmp(menuC2, "mute", 4) == 0 ) {
      sendCode(SamsungRemote::mute);
    } else if ( strncmp(menuC2, "menu", 4) == 0 ) {
      
    } else if ( strncmp(menuC2, "exit", 4) == 0 ) {
      
    } else if ( strncmp(menuC2, "info", 4) == 0 ) {
      
    } else if ( strncmp(menuC2, "tools", 4) == 0 ) {
      
    } else if ( strncmp(menuC2, "up", 4) == 0 ) {
      
    } else if ( strncmp(menuC2, "down", 4) == 0 ) {
      
    } else if ( strncmp(menuC2, "left", 4) == 0 ) {
      
    } else if ( strncmp(menuC2, "right", 4) == 0 ) {
      
    } else if ( strncmp(menuC2, "play", 4) == 0 ) {
      
    } else if ( strncmp(menuC2, "pause", 4) == 0 ) {
      
    } else if ( strncmp(menuC2, "stop", 4) == 0 ) {
      
    }
  }
}

unsigned long lastAttempt = 0;
void reconnect() {
  if ( lastAttempt = 0 ) {
    lastAttempt = millis();
  } else if ( millis() - lastAttempt <= 15000 ) {
    return;
  }

  lastAttempt = millis();
  
  // Loop until we're reconnected
  if (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("espslpd", "hello world");
      // ... and resubscribe
      client.subscribe("espslpd");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      // delay(5000);
    }
  }
}

unsigned long Timer = 0;

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

  if (false) {
  } else if (Timer != 0 && millis() - Timer >= 15000) {
    Serial.println("Reconnecting websocket.");
    Timer = 0;
  } else if (false && Timer == 0) {
    Serial.println("Client disconnected. Scheduling reconnect attempt.");
    Timer = millis();
  }

  
  if (!client.connected()) {
    reconnect();
  } else {
    // Client is connected.
    client.loop();
  
    long now = millis();
    if (now - lastMsg > 60000) {
      lastMsg = now;
      ++value;
      snprintf (msg, 75, "keepalive #%ld", value);
      Serial.print("Publish message: ");
      Serial.println(msg);
      client.publish("espslpd", msg);
    }
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
