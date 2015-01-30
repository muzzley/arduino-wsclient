#ifndef WSCLIENT_H_
#define WSCLIENT_H_

#include "Callback.h"
#include <Ethernet.h>

class WSClient{
  public:
    enum Event{ ON_CONNECT, ON_CLOSE, ON_ERROR, ON_DATA };
    void connect(char* host, int port = 80, char* path = "/");
    bool connected();
    void disconnect();
    void setEventsHandler(Delegate<void, Event, char*> *events_handler);
    void send(char* data);
    void getNextPacket();

  private:
    void readLine(char* buffer);
    bool handshake(char* host, char* path);
    byte getNext();
    void getStringTableItem(char* buffer, int index);
    size_t base64Encode(byte* src, size_t srclength, char* target, size_t targetsize);
    Delegate<void, Event, char*> *_events_handler;
    EthernetClient _socket;
    char* _packet;
    unsigned int _packetLength;
    byte _opCode;
};

const char b64Alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
#endif
