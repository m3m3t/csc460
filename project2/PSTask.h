/*
 PSTask.h - A simple client for .
  Nicholas O'Leary
  http://knolleary.net
*/

#ifndef PSTask_h
#define PSTask_h

#include <Arduino.h>
#include "Client.h"

// _MAX_PACKET_SIZE : Maximum packet size
#define PS_MAX_PACKET_SIZE 128

// _KEEPALIVE : keepAlive interval in Seconds
#define PS_KEEPALIVE 15

#define PROTOCOLVERSION 1
#define CONNECT     1 << 4  // Client request to connect to Server
#define CONNACK     2 << 4  // Connect Acknowledgment
#define PUBLISH     3 << 4  // Publish message
#define PUBACK      4 << 4  // Publish Acknowledgment
#define PUBREC      5 << 4  // Publish Received (assured delivery part 1)
#define PUBREL      6 << 4  // Publish Release (assured delivery part 2)
#define PUBCOMP     7 << 4  // Publish Complete (assured delivery part 3)
#define SUBSCRIBE   8 << 4  // Client Subscribe request
#define SUBACK      9 << 4  // Subscribe Acknowledgment
#define UNSUBSCRIBE 10 << 4 // Client Unsubscribe request
#define UNSUBACK    11 << 4 // Unsubscribe Acknowledgment
#define PINGREQ     12 << 4 // PING Request
#define PINGRESP    13 << 4 // PING Response
#define DISCONNECT  14 << 4 // Client is Disconnecting
#define Reserved    15 << 4 // Reserved

#define QOS0        (0 << 1)
#define QOS1        (1 << 1)
#define QOS2        (2 << 1)

class PSTask {
private:
   Client* _client;
   uint8_t buffer[_MAX_PACKET_SIZE];
   uint16_t nextMsgId;
   unsigned long lastOutActivity;
   unsigned long lastInActivity;
   bool pingOutstanding;
   void (*callback)(char*,uint8_t*,unsigned int);
   uint16_t readPacket(uint8_t*);
   uint8_t readByte();
   boolean write(uint8_t header, uint8_t* buf, uint16_t length);
   uint16_t writeString(char* string, uint8_t* buf, uint16_t pos);
public:
   PSTask();
   PSTask(void(*)(char*,uint8_t*,unsigned int),Client& client);
   boolean connect(char *);
   void disconnect();
   boolean publish(char *, char *);
   boolean publish(char *, uint8_t *, unsigned int);
   boolean publish(char *, uint8_t *, unsigned int, boolean);
   boolean publish_P(char *, uint8_t PROGMEM *, unsigned int, boolean);
   boolean subscribe(char *);
   boolean subscribe(char *, uint8_t qos);
   boolean unsubscribe(char *);
   boolean loop();
   boolean connected();
};


#endif
