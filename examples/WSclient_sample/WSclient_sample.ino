#include <Ethernet.h>
#include <WSClient.h>
#include <SPI.h>

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xAD };
WSClient client;

void onMessage(WSClient::Event event, char* msg) {
  Serial.println("--- Got message ----");
  switch(event){
    
    case WSClient::ON_CONNECT:
      Serial.println(F("[ok]Connected"));
      break;
      
    case WSClient::ON_CLOSE:
      Serial.println(F("[Disconnected]"));
      break;
      
    case WSClient::ON_ERROR:
      Serial.println(F("[Error] "));
      Serial.println(msg);
      break;

    case WSClient::ON_DATA:
      Serial.println(F("[Data] "));
      Serial.println(msg);
      break;
      
    default:
      Serial.print(F("Unknown: "));
      Serial.println(msg);
      break;
  }
}

void setup() {
  Ethernet.begin(mac);
  Serial.begin(9600);
  
  Serial.println(F("Starting Handler"));
  client.setEventsHandler(new Delegate<void, WSClient::Event, char*>(&onMessage));
  delay(2000);
  
  Serial.println(F("Connecting"));
  client.connect("ws.websocketstest.com", 80, "/");
  delay(500);
  
  client.send("Hello world");
  Serial.println(F("Sent Hello World"));
}

void loop() {
  delay(50);
  client.getNextPacket();
}


