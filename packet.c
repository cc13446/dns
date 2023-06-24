//
// Created by oem on 23-6-24.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include "packet.h"
#include "debug.h"


size_t decoderOther(struct DnsOther* other, void* buf, void* sourceBuf, uint16_t len) {
    size_t offset = 0;
    other = malloc(sizeof(struct DnsOther) * len);
    for (int i = 0; i < len; i++) {
        if ((*(uint8_t *)buf) == 0xc0) {
            buf += sizeof(uint8_t);
            offset += sizeof(uint8_t);
            uint8_t off = *(uint8_t *)buf;
            buf += sizeof(uint8_t);
            offset += sizeof(uint8_t);
            void* b = sourceBuf + off;
            size_t l = strlen((const char *) b) + 1;
            other[i].name = malloc(sizeof(char) * l);
            memcpy(other[i].name, b, l * sizeof(char));
        } else {
            size_t l = strlen((const char *) buf) + 1;
            other[i].name = malloc(sizeof(char) * l);
            memcpy(other[i].name, buf, l * sizeof(char));
            buf += l * sizeof (char);
            offset += l * sizeof (char);
        }
        size_t nameLen = strlen(other[i].name) + 1;
        char readableName[nameLen];
        getReadableName(other[i].name, readableName, nameLen - 1);
        ddbg("Other name %ld %s", strlen(readableName), readableName)

        other[i].type = ntohs(*((uint16_t *) buf));
        buf += sizeof (uint16_t);
        offset += sizeof (uint16_t);
        other[i].class = ntohs(*((uint16_t *) buf));
        buf += sizeof (uint16_t);
        offset += sizeof (uint16_t);
        other[i].ttl = ntohs(*((uint32_t *) buf));
        buf += sizeof (uint32_t);
        offset += sizeof (uint32_t);
        other[i].length = ntohs(*((uint16_t *) buf));
        buf += sizeof (uint16_t);
        offset += sizeof (uint16_t);
        other[i].resource = malloc(sizeof(char) * other[i].length);
        memcpy(other[i].resource, buf, other[i].length * sizeof (char));
        buf += other[i].length * sizeof (char);
        offset += other[i].length * sizeof (char);
        ddbg("Decoder dns resource %s %d %d %d", readableName, other[i].type, other[i].class, other[i].length)
    }
    return offset;
}

struct DnsPacket* decoder(void* buf) {
    void* sourceBuf = buf;
    struct DnsPacket* res = malloc(sizeof(struct DnsPacket));
    struct DnsHeader* tempHeader = (struct DnsHeader*) buf;
    res->header.id = ntohs(tempHeader->id);
    res->header.flags = ntohs(tempHeader->flags);
    res->header.questionCount = ntohs(tempHeader->questionCount);
    res->header.answerCount = ntohs(tempHeader->answerCount);
    res->header.authorityCount = ntohs(tempHeader->authorityCount);
    res->header.additionalCount = ntohs(tempHeader->additionalCount);

    ddbg("Decoder dns header %d %d %d %d %d %d", res->header.id,  res->header.flags,  res->header.questionCount,  res->header.answerCount,  res->header.authorityCount,  res->header.additionalCount)

    buf += sizeof(struct DnsHeader);
    res->questions = malloc(sizeof(struct DnsQuestion) * res->header.questionCount);
    for (int i = 0; i < res->header.questionCount; i++) {
        size_t nameLen = strlen((const char *) buf) + 1;
        res->questions[i].name = malloc(sizeof (char) * nameLen);
        memcpy(res->questions[i].name, buf, nameLen * sizeof (char));
        char readableName[nameLen];
        getReadableName(res->questions[i].name, readableName, nameLen - 1);
        ddbg("Question name %ld %s", strlen(readableName), readableName)

        buf += nameLen * sizeof (char);
        res->questions[i].type = ntohs(*((uint16_t *) buf));
        buf += sizeof (uint16_t);
        res->questions[i].class = ntohs(*((uint16_t *) buf));
        buf += sizeof (uint16_t);
        ddbg("Decoder dns question %s %d %d", readableName, res->questions[i].type, res->questions[i].class)
    }

    size_t offset;
    offset = decoderOther(res->answers, buf, sourceBuf, res->header.answerCount);
    buf += offset;
    offset = decoderOther(res->authorities, buf, sourceBuf, res->header.authorityCount);
    buf += offset;
    decoderOther(res->additional, buf, sourceBuf, res->header.additionalCount);

    return res;
}

void getReadableName(const char* src, char* dst, size_t len) {
    memset(dst, 0, len + 1);
    for (int i = 0; i < len; i++) {
        if ((src[i] >= 'a' && src[i] <= 'z')
            || (src[i] >= 'A' && src[i] <= 'Z')
            || (src[i] >= '0' && src[i] <= '9')
            || src[i] == '-') {
            dst[i] = src[i];
        } else {
            dst[i] = '.';
        }
    }
}

void freePacket(struct DnsPacket* packet) {
    for (int i = 0; i < packet->header.questionCount; i++) {
        free(packet->questions[i].name);
    }
    free(packet->questions);
    for (int i = 0; i < packet->header.answerCount; i++) {
        free(packet->answers[i].name);
        free(packet->answers[i].resource);
    }
    free(packet->answers);
    for (int i = 0; i < packet->header.authorityCount; i++) {
        free(packet->authorities[i].name);
        free(packet->authorities[i].resource);
    }
    free(packet->authorities);
    for (int i = 0; i < packet->header.additionalCount; i++) {
        free(packet->additional[i].name);
        free(packet->additional[i].resource);
    }
    free(packet->additional);
}

char getQR(struct DnsPacket* packet) {
    return (packet->header.flags & 0x8000) == 0 ? 'Q' : 'R';
}

void setPacketId(void* buf, uint16_t id) {
    struct DnsHeader* header = (struct DnsHeader*) buf;
    header->id = id;
}