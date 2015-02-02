#include <WSClient.h>

//#define GALILEO_ARDUINO

#ifdef GALILEO_ARDUINO
  const char handshake_line1a[] PROGMEM = "GET ";
  const char handshake_line1b[] PROGMEM = " HTTP/1.1";
  const char handshake_line2[] PROGMEM = "Upgrade: websocket";
  const char handshake_line3[] PROGMEM = "Connection: Upgrade";
  const char handshake_line4[] PROGMEM = "Host: ";
  const char handshake_line5[] PROGMEM = "Origin: ArduinoWSClient";
  const char handshake_line6[] PROGMEM = "Sec-WebSocket-Version: 13";
  const char handshake_line7[] PROGMEM = "Sec-WebSocket-Key: ";
  const char handshake_success_response[] PROGMEM = "HTTP/1.1 101";
  const char upg_err_msg[] = "Failed socket upgrade";
  const char connect_err_msg[] = "Failed to connect";
#else
  prog_char handshake_line1a[] PROGMEM = "GET ";
  prog_char handshake_line1b[] PROGMEM = " HTTP/1.1";
  prog_char handshake_line2[] PROGMEM = "Upgrade: websocket";
  prog_char handshake_line3[] PROGMEM = "Connection: Upgrade";
  prog_char handshake_line4[] PROGMEM = "Host: ";
  prog_char handshake_line5[] PROGMEM = "Origin: http://www.websocket.org";
  prog_char handshake_line6[] PROGMEM = "Sec-WebSocket-Version: 13";
  prog_char handshake_line7[] PROGMEM = "Sec-WebSocket-Key: ";
  prog_char handshake_success_response[] PROGMEM = "HTTP/1.1 101";
  prog_char upg_err_msg[] = "Failed socket upgrade";
  prog_char connect_err_msg[] = "Failed to connect";
#endif


#ifdef GALILEO_ARDUINO
  const char *WSClientStringTable[] =
  {
    handshake_line1a,
    handshake_line1b,
    handshake_line2,
    handshake_line3,
    handshake_line4,
    handshake_line5,
    handshake_line6,
    handshake_line7,
    handshake_success_response,
    upg_err_msg,
    connect_err_msg
  };
#else
  PROGMEM const char *WSClientStringTable[] =
  {
    handshake_line1a,
    handshake_line1b,
    handshake_line2,
    handshake_line3,
    handshake_line4,
    handshake_line5,
    handshake_line6,
    handshake_line7,
    handshake_success_response,
    upg_err_msg,
    connect_err_msg
  };
#endif


void WSClient::getStringTableItem(char* buffer, int index){
  #ifdef GALILEO_ARDUINO 
    strcpy(buffer, WSClientStringTable[index]);
  #else
    strcpy_P(buffer, (char*)pgm_read_word(&(WSClientStringTable[index])));
  #endif
}

void WSClient::readLine(char* buffer){
  char character;
  int i = 0;

  while(_socket.available() > 0 && (character = _socket.read()) != '\n') {
    if(character != '\r' && character != -1) {
      buffer[i++] = character;
    }
  }
  buffer[i] = 0x0;
}

bool WSClient::handshake(char* host, char* path){
  #ifndef GALILEO_ARDUINO
    randomSeed(analogRead(0));
  #endif

  char line[128];
  bool result = false;
  char response[12];
  int maxAttempts = 300, attempts = 0;

  byte bytes[16];
  for(int i = 0; i < 16; i++) {
    bytes[i] = 255 * random();
  } 

  char buffer[45];
  getStringTableItem(buffer, 0);
  _socket.print(buffer);
  _socket.print(path);
  getStringTableItem(buffer, 1);
  _socket.println(buffer);
  getStringTableItem(buffer, 2);
  _socket.println(buffer);
  getStringTableItem(buffer, 3);
  _socket.println(buffer);
  getStringTableItem(buffer, 4);
  _socket.print(buffer);
  _socket.println(host);
  getStringTableItem(buffer, 5);
  _socket.println(buffer);
  getStringTableItem(buffer, 6);
  _socket.println(buffer);
  getStringTableItem(buffer, 7);
  _socket.print(buffer);
   base64Encode(bytes, 16, buffer, 45);
  _socket.println(buffer);
  _socket.println("");
   
  while(_socket.available() == 0 && attempts < maxAttempts){
    delay(100);
    attempts++;
  }
  
  while(true) {
    readLine(line);
    Serial.println(line);
    if(strcmp(line, "") == 0) { break;}
    getStringTableItem(response, 8);
    if(strncmp(line, response, 12) == 0) {result = true;}
  }
  
  return result;
}

byte WSClient::getNext() {
  while(_socket.available() == 0);
  byte b = _socket.read();
  return b;
}

void WSClient::connect(char* host, int port, char* path){
  if(_events_handler == NULL){ return; }
  if(_socket.connected()){
    disconnect();
  }
  if(_socket.connect(host, port)){
    if(!handshake(host, path)){
      char msg[21];
      getStringTableItem(msg, 9);
      (*_events_handler)(ON_ERROR, msg);
      disconnect();
    }else{
      (*_events_handler)(ON_CONNECT, NULL);
    }
   

  }else{
    char msg[21];
    getStringTableItem(msg, 10);
    (*_events_handler)(ON_ERROR, msg);
    disconnect();
  }
}

void WSClient::disconnect(){
  if(_socket.connected()){
    _socket.write((uint8_t) 0x87);
    _socket.write((uint8_t) 0x00);
  }
  _socket.flush();
  _socket.stop();
  (*_events_handler)(ON_CLOSE, NULL);
}

bool WSClient::connected(){
  return _socket.connected();
}

void WSClient::send(char* data){
  int len = strlen(data);
  _socket.write(0x81);
  if(len > 125) {
    _socket.write(0xFE);
    _socket.write(byte(len >> 8));
    _socket.write(byte(len & 0xFF));
  } else {
    _socket.write(0x80 | byte(len));
  }
  for(int i = 0; i < 4; i++) {
    _socket.write((byte)0x00);
  }
  _socket.print(data);
}

void WSClient::setEventsHandler(Delegate<void, Event, char*>* events_handler){
  _events_handler = events_handler;
}

void WSClient::getNextPacket() {
  if(!connected()) return;

  if(_socket.available() > 0) {
    byte hdr = getNext();
    bool fin = hdr & 0x80;
    int opCode = hdr & 0x0F;
    hdr = getNext();
    bool mask = hdr & 0x80;
    int len = hdr & 0x7F;
    if(len == 126) {
      len = getNext();
      len <<= 8;
      len += getNext();
    } else if (len == 127) {
      len = getNext();
      for(int i = 0; i < 7; i++) {
        len <<= 8;
        len += getNext();
      }
    }
    if(mask) {
      for(int i = 0; i < 4; i++) { getNext(); }
    }

    if(mask) {
      free(_packet);
      return;
    }

    if(!fin) {
      if(_packet == NULL) {
        _packet = (char*) malloc(len);
        for(int i = 0; i < len; i++) { _packet[i] = getNext(); }
        _packetLength = len;
        _opCode = opCode;
      } else {
        int copyLen = _packetLength;
        _packetLength += len;
        char *temp = _packet;
        _packet = (char*)malloc(_packetLength);
        for(int i = 0; i < _packetLength; i++) {
          if(i < copyLen) {
            _packet[i] = temp[i];
          } else {
            _packet[i] = getNext();
          }
        }
        free(temp);
      }
      return;
    }

    if(_packet == NULL) {
      _packet = (char*) malloc(len + 1);
      for(int i = 0; i < len; i++) { _packet[i] = getNext(); }
      _packet[len] = 0x0;
    } else {
      int copyLen = _packetLength;
      _packetLength += len;
      char *temp = _packet;
      _packet = (char*) malloc(_packetLength + 1);
      for(int i = 0; i < _packetLength; i++) {
        if(i < copyLen) {
          _packet[i] = temp[i];
        } else {
          _packet[i] = getNext();
        }
      }
      _packet[_packetLength] = 0x0;
      free(temp);
    }
    
    if(opCode == 0 && _opCode > 0) {
      opCode = _opCode;
      _opCode = 0;
    }

    switch(opCode) {
      case 0x01:
        (*_events_handler)(ON_DATA, _packet);
        break;
        
      case 0x09:
        _socket.write(0x8A);
        _socket.write(byte(0x00));
        break;
        
      case 0x08:
        unsigned int code = ((byte)_packet[0] << 8) + (byte)_packet[1];
        disconnect();
        break;
    }
    free(_packet);
    _packet = NULL;
  }
}

size_t WSClient::base64Encode(byte* src, size_t srclength, char* target, size_t targsize) {
  size_t datalength = 0;
  char input[3];
  char output[4];
  size_t i;

  while (2 < srclength) {
    input[0] = *src++;
    input[1] = *src++;
    input[2] = *src++;
    srclength -= 3;

    output[0] = input[0] >> 2;
    output[1] = ((input[0] & 0x03) << 4) + (input[1] >> 4);
    output[2] = ((input[1] & 0x0f) << 2) + (input[2] >> 6);
    output[3] = input[2] & 0x3f;

    if(datalength + 4 > targsize) { return (-1); }

    target[datalength++] = b64Alphabet[output[0]];
    target[datalength++] = b64Alphabet[output[1]];
    target[datalength++] = b64Alphabet[output[2]];
    target[datalength++] = b64Alphabet[output[3]];
  }

  if(0 != srclength) {
    input[0] = input[1] = input[2] = '\0';
    for (i = 0; i < srclength; i++) { input[i] = *src++; }

    output[0] = input[0] >> 2;
    output[1] = ((input[0] & 0x03) << 4) + (input[1] >> 4);
    output[2] = ((input[1] & 0x0f) << 2) + (input[2] >> 6);

    if(datalength + 4 > targsize) { return (-1); }

    target[datalength++] = b64Alphabet[output[0]];
    target[datalength++] = b64Alphabet[output[1]];
    if(srclength == 1) {
      target[datalength++] = '=';
    } else {
      target[datalength++] = b64Alphabet[output[2]];
    }
    target[datalength++] = '=';
  }
  if(datalength >= targsize) { return (-1); }
  target[datalength] = '\0';
  return (datalength);
}
