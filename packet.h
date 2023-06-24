//
// Created by cc on 23-6-24.
//

#ifndef DNS_PACKET_H
#define DNS_PACKET_H

#include <stdint.h>

struct DnsHeader {
    uint16_t id;
    uint16_t flags;
    uint16_t questionCount;
    uint16_t answerCount;
    uint16_t authorityCount;
    uint16_t additionalCount;
};

struct DnsQuestion {
    char* name;
    uint16_t type;
    uint16_t class;
};

struct DnsOther {
    char* name;
    uint16_t type;
    uint16_t class;
    uint32_t ttl;
    uint16_t length;
    char* resource;
};

struct DnsPacket{
    struct DnsHeader header;
    struct DnsQuestion* questions;
    struct DnsOther* answers;
    struct DnsOther* authorities;
    struct DnsOther* additional;

};

struct DnsPacket* decoder(void* buf);
void freePacket(struct DnsPacket* packet);
void getReadableName(const char* src, char* dst, size_t len);
char getQR(struct DnsPacket* packet);

// 设置dns报文的事务id
void setPacketId(void* buf, uint16_t id);

#endif //DNS_PACKET_H
