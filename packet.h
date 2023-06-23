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

#endif //DNS_PACKET_H
