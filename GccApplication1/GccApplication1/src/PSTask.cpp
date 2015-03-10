/*
NOTE:  all code with _client is just a generic interface until I write it 
functions needed:
stop()
connect()
available()
read()
write()
connected()
*/

#include "PSTask.h"
#include <string.h>

PSTask::PSTask() {
   this->_client = NULL;
}

PSTask::PSTask(void (*callback)(char*,uint8_t*,unsigned int), Client& client) {
   this->_client = &client;
   this->callback = callback;
}


boolean PSTask::connect(char *id) {

   if (!connected()) {
      int result = 0;
      result = _client->connect();
      
      if (result) {
         nextMsgId = 1;
         
         uint8_t d[4] = {0x00,0x06,PROTOCOLVERSION,0x02};
         // Leave room in the buffer for header and variable length field
         uint16_t length = 5;
         unsigned int j;
         for (j = 0;j<4;j++) {
            buffer[length++] = d[j];
         }
         buffer[length++] = ((PS_KEEPALIVE) >> 8);
         buffer[length++] = ((PS_KEEPALIVE) & 0xFF);
         length = writeString(id,buffer,length);
         write(CONNECT,buffer,length-5);
         lastInActivity = lastOutActivity = millis();
         
         while (!_client->available()) {
            unsigned long t = millis();
            if (t-lastInActivity > PS_KEEPALIVE*1000UL) {
               _client->stop();
               return false;
            }
         }
         uint8_t llen;
         uint16_t len = readPacket(&llen);
         
         if (len == 4 && buffer[3] == 0) {
            lastInActivity = millis();
            pingOutstanding = false;
            return true;
         }
      }
      _client->stop();
   }
   return false;
}

uint8_t PSTask::readByte() {
   while(!_client->available()) {}
   return _client->read();
}

uint16_t PSTask::readPacket(uint8_t* lengthLength) {
   uint16_t len = 0;
   buffer[len++] = readByte();
   bool isPublish = (buffer[0]&0xF0) == PUBLISH;
   uint32_t multiplier = 1;
   uint16_t length = 0;
   uint8_t digit = 0;
   uint16_t skip = 0;
   uint8_t start = 0;
   
   do {
      digit = readByte();
      buffer[len++] = digit;
      length += (digit & 127) * multiplier;
      multiplier *= 128;
   } while ((digit & 128) != 0);
   *lengthLength = len-1;

   if (isPublish) {
      // Read in topic length to calculate bytes to skip over 
      buffer[len++] = readByte();
      buffer[len++] = readByte();
      skip = (buffer[*lengthLength+1]<<8)+buffer[*lengthLength+2];
      start = 2;
      if (buffer[0]&QOS1) {
         // skip message id
         skip += 2;
      }
   }

   for (uint16_t i = start;i<length;i++) {
      digit = readByte();
      if (len < PS_MAX_PACKET_SIZE) {
         buffer[len] = digit;
      }
      len++;
   }
   
   if (len > PS_MAX_PACKET_SIZE) {
       len = 0; // This will cause the packet to be ignored.
   }

   return len;
}

boolean PSTask::loop() {
   if (connected()) {
      unsigned long t = millis();
      if ((t - lastInActivity > PS_KEEPALIVE*1000UL) || (t - lastOutActivity > PS_KEEPALIVE*1000UL)) {
         if (pingOutstanding) {
            _client->stop();
            return false;
         } else {
            buffer[0] = PINGREQ;
            buffer[1] = 0;
            _client->write(buffer,2);
            lastOutActivity = t;
            lastInActivity = t;
            pingOutstanding = true;
         }
      }
      if (_client->available()) {
         uint8_t llen;
         uint16_t len = readPacket(&llen);
         uint16_t msgId = 0;
         uint8_t *payload;
         if (len > 0) {
            lastInActivity = t;
            uint8_t type = buffer[0]&0xF0;
            if (type == PUBLISH) {
               if (callback) {
                  uint16_t tl = (buffer[llen+1]<<8)+buffer[llen+2];
                  char topic[tl+1];
                  for (uint16_t i=0;i<tl;i++) {
                     topic[i] = buffer[llen+3+i];
                  }
                  topic[tl] = 0;
                  // msgId only present for QOS>0
                  if ((buffer[0]&0x06) == QOS1) {
                    msgId = (buffer[llen+3+tl]<<8)+buffer[llen+3+tl+1];
                    payload = buffer+llen+3+tl+2;
                    callback(topic,payload,len-llen-3-tl-2);
                    
                    buffer[0] = PUBACK;
                    buffer[1] = 2;
                    buffer[2] = (msgId >> 8);
                    buffer[3] = (msgId & 0xFF);
                    _client->write(buffer,4);
                    lastOutActivity = t;

                  } else {
                    payload = buffer+llen+3+tl;
                    callback(topic,payload,len-llen-3-tl);
                  }
               }
            } else if (type == PINGREQ) {
               buffer[0] = PINGRESP;
               buffer[1] = 0;
               _client->write(buffer,2);
            } else if (type == PINGRESP) {
               pingOutstanding = false;
            }
         }
      }
      return true;
   }
   return false;
}

boolean PSTask::publish(char* topic, char* payload) {
   return publish(topic,(uint8_t*)payload,strlen(payload),false);
}

boolean PSTask::publish(char* topic, uint8_t* payload, unsigned int plength) {
   return publish(topic, payload, plength, false);
}

boolean PSTask::publish(char* topic, uint8_t* payload, unsigned int plength, boolean retained) {
   if (connected()) {
      // Leave room in the buffer for header and variable length field
      uint16_t length = 5;
      length = writeString(topic,buffer,length);
      uint16_t i;
      for (i=0;i<plength;i++) {
         buffer[length++] = payload[i];
      }
      uint8_t header = PUBLISH;
      if (retained) {
         header |= 1;
      }
      return write(header,buffer,length-5);
   }
   return false;
}

boolean PSTask::publish_P(char* topic, uint8_t* PROGMEM payload, unsigned int plength, boolean retained) {
   uint8_t llen = 0;
   uint8_t digit;
   unsigned int rc = 0;
   uint16_t tlen;
   unsigned int pos = 0;
   unsigned int i;
   uint8_t header;
   unsigned int len;
   
   if (!connected()) {
      return false;
   }
   
   tlen = strlen(topic);
   
   header = PUBLISH;
   if (retained) {
      header |= 1;
   }
   buffer[pos++] = header;
   len = plength + 2 + tlen;
   do {
      digit = len % 128;
      len = len / 128;
      if (len > 0) {
         digit |= 0x80;
      }
      buffer[pos++] = digit;
      llen++;
   } while(len>0);
   
   pos = writeString(topic,buffer,pos);
   
   rc += _client->write(buffer,pos);
   
   for (i=0;i<plength;i++) {
      rc += _client->write((char)pgm_read_byte_near(payload + i));
   }
   
   lastOutActivity = millis();
   
   return rc == tlen + 4 + plength;
}

boolean PSTask::write(uint8_t header, uint8_t* buf, uint16_t length) {
   uint8_t lenBuf[4];
   uint8_t llen = 0;
   uint8_t digit;
   uint8_t pos = 0;
   uint8_t rc;
   uint8_t len = length;
   do {
      digit = len % 128;
      len = len / 128;
      if (len > 0) {
         digit |= 0x80;
      }
      lenBuf[pos++] = digit;
      llen++;
   } while(len>0);

   buf[4-llen] = header;
   for (int i=0;i<llen;i++) {
      buf[5-llen+i] = lenBuf[i];
   }
   rc = _client->write(buf+(4-llen),length+1+llen);
   
   lastOutActivity = millis();
   return (rc == 1+llen+length);
}

boolean PSTask::subscribe(char* topic) {
  return subscribe(topic, 0);
}

boolean PSTask::subscribe(char* topic, uint8_t qos) {
   if (qos < 0 || qos > 1)
     return false;

   if (connected()) {
      // Leave room in the buffer for header and variable length field
      uint16_t length = 5;
      nextMsgId++;
      if (nextMsgId == 0) {
         nextMsgId = 1;
      }
      buffer[length++] = (nextMsgId >> 8);
      buffer[length++] = (nextMsgId & 0xFF);
      length = writeString(topic, buffer,length);
      buffer[length++] = qos;
      return write(SUBSCRIBE|QOS1,buffer,length-5);
   }
   return false;
}

boolean PSTask::unsubscribe(char* topic) {
   if (connected()) {
      uint16_t length = 5;
      nextMsgId++;
      if (nextMsgId == 0) {
         nextMsgId = 1;
      }
      buffer[length++] = (nextMsgId >> 8);
      buffer[length++] = (nextMsgId & 0xFF);
      length = writeString(topic, buffer,length);
      return write(UNSUBSCRIBE|QOS1,buffer,length-5);
   }
   return false;
}

void PSTask::disconnect() {
   buffer[0] = DISCONNECT;
   buffer[1] = 0;
   _client->write(buffer,2);
   _client->stop();
   lastInActivity = lastOutActivity = millis();
}

uint16_t PSTask::writeString(char* string, uint8_t* buf, uint16_t pos) {
   char* idp = string;
   uint16_t i = 0;
   pos += 2;
   while (*idp) {
      buf[pos++] = *idp++;
      i++;
   }
   buf[pos-i-2] = (i >> 8);
   buf[pos-i-1] = (i & 0xFF);
   return pos;
}


boolean PSTask::connected() {
   boolean rc;
   if (_client == NULL ) {
      rc = false;
   } else {
      rc = (int)_client->connected();
      if (!rc) _client->stop();
   }
   return rc;
}

