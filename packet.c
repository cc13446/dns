//
// Created by oem on 23-6-24.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "packet.h"
#include "debug.h"


size_t decoderOther(struct DnsOther* other, void* buf, void* sourceBuf, uint16_t len) {
    size_t offset = 0;
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
    res->answers = malloc(sizeof(struct DnsOther) * res->header.answerCount);
    offset = decoderOther(res->answers, buf, sourceBuf, res->header.answerCount);
    buf += offset;
    res->authorities = malloc(sizeof(struct DnsOther) * res->header.authorityCount);
    offset = decoderOther(res->authorities, buf, sourceBuf, res->header.authorityCount);
    buf += offset;
    res->additional = malloc(sizeof(struct DnsOther) * res->header.additionalCount);
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

void getNetName(char* src, char* dst, size_t srcLen) {
    memset(dst, 0, srcLen + 2);
    char* temp = src + srcLen - 1;
    dst = dst + srcLen;

    char count = 0;
    for (; temp >= src; temp--, dst--) {
        if ((*temp >= 'a' && *temp <= 'z')
            || (*temp >= 'A' && *temp <= 'Z')
            || (*temp >= '0' && *temp <= '9')
            || *temp == '-') {
            *dst = *temp;
            count++;
        } else {
            *dst = count;
            count = 0;
        }
    }
    *dst = count;
}

void freePacket(struct DnsPacket* packet) {
    for (int i = 0; i < packet->header.questionCount; i++) {
        if (packet->questions[i].name) {
            free(packet->questions[i].name);
        }
    }
    free(packet->questions);
    for (int i = 0; i < packet->header.answerCount; i++) {
        if (packet->answers[i].name) {
            free(packet->answers[i].name);
        }
        if (packet->answers[i].resource) {
            free(packet->answers[i].resource);
        }
    }
    free(packet->answers);
    for (int i = 0; i < packet->header.authorityCount; i++) {
        if (packet->authorities[i].name) {
            free(packet->authorities[i].name);
        }
        if (packet->authorities[i].resource) {
            free(packet->authorities[i].resource);
        }
    }
    free(packet->authorities);
    for (int i = 0; i < packet->header.additionalCount; i++) {
        if (packet->additional[i].name) {
            free(packet->additional[i].name);
        }
        if (packet->additional[i].resource) {
            free(packet->additional[i].resource);
        }
    }
    free(packet->additional);
    free(packet);
}

char getQR(struct DnsPacket* packet) {
    return (packet->header.flags & 0x8000) == 0 ? 'Q' : 'R';
}

void setPacketId(void* buf, uint16_t id) {
    struct DnsHeader* header = (struct DnsHeader*) buf;
    header->id = id;
}

size_t encoderOther(struct DnsOther* other, void* buf, uint16_t len) {
    size_t offset = 0;
    for (int i = 0; i < len; i++) {
        size_t l = strlen(other[i].name) + 1;
        memcpy(buf, other[i].name, l * sizeof(char));
        buf += l * sizeof (char);
        offset += l * sizeof (char);
        char readableName[l];
        getReadableName(other[i].name, readableName, l - 1);
        ddbg("Other name %ld %s", strlen(readableName), readableName)

        *((uint16_t *) buf) = htons(other[i].type);
        buf += sizeof (uint16_t);
        offset += sizeof (uint16_t);
        *((uint16_t *) buf) = htons(other[i].class);
        buf += sizeof (uint16_t);
        offset += sizeof (uint16_t);
        *((uint32_t *) buf) = htons(other[i].ttl);
        buf += sizeof (uint32_t);
        offset += sizeof (uint32_t);
        *((uint16_t *) buf) = htons(other[i].length);
        buf += sizeof (uint16_t);
        offset += sizeof (uint16_t);
        memcpy(buf, other[i].resource, other[i].length * sizeof (char));
        buf += other[i].length * sizeof (char);
        offset += other[i].length * sizeof (char);
        ddbg("Decoder dns resource %s %d %d %d", readableName, other[i].type, other[i].class, other[i].length)
    }
    return offset;
}

size_t encoder(struct DnsPacket* packet, void* buf) {
    size_t offset = 0;
    struct DnsHeader* temp = (struct DnsHeader*) buf;
    temp->id = htons(packet->header.id);
    temp->flags = htons(packet->header.flags);
    temp->questionCount = htons(packet->header.questionCount);
    temp->answerCount = htons(packet->header.answerCount);
    temp->authorityCount = htons(packet->header.authorityCount);
    temp->additionalCount = htons(packet->header.additionalCount);
    ddbg("Encoder dns header %d %d %d %d %d %d", temp->id,  temp->flags,  temp->questionCount,  temp->answerCount, temp->authorityCount, temp->additionalCount)

    buf += sizeof(struct DnsHeader);
    offset += sizeof(struct DnsHeader);
    for (int i = 0; i < packet->header.questionCount; i++) {
        size_t nameLen = strlen(packet->questions[i].name) + 1;
        memcpy(buf, packet->questions[i].name, nameLen * sizeof (char));
        char readableName[nameLen];
        getReadableName(packet->questions[i].name, readableName, nameLen - 1);
        ddbg("Question name %ld %s", strlen(readableName), readableName)

        buf += nameLen * sizeof (char);
        offset += nameLen * sizeof (char);
        *((uint16_t *) buf) = htons(packet->questions[i].type);
        buf += sizeof (uint16_t);
        offset += sizeof (uint16_t);
        *((uint16_t *) buf) = htons(packet->questions[i].class);
        buf += sizeof (uint16_t);
        offset += sizeof (uint16_t);
        ddbg("Decoder dns question %s %d %d", readableName, packet->questions[i].type, packet->questions[i].class)
    }

    size_t off;
    off = encoderOther(packet->answers, buf, packet->header.answerCount);
    buf += off;
    offset += off;
    off = encoderOther(packet->authorities, buf, packet->header.authorityCount);
    buf += offset;
    offset += off;
    off = encoderOther(packet->additional, buf, packet->header.additionalCount);
    offset += off;
    return offset;
}

struct DnsPacket* getAnswerPacker(char* addr, char* ip) {
    struct DnsPacket* res = malloc(sizeof (struct DnsPacket));
    res->header.id = 0;
    res->header.flags = strcmp(ip, "0.0.0.0") == 0 ? 0x8183 : 0x8180;
    res->header.questionCount = 1;
    res->header.answerCount = strcmp(ip, "0.0.0.0") == 0 ? 0 : 1;
    res->questions = malloc(sizeof(struct DnsQuestion));
    size_t nameLen = strlen(addr);
    res->questions->name = malloc(nameLen * sizeof (char) + 1);
    strcpy(res->questions->name, addr);
    res->questions->type = 0x0001;
    res->questions->class = 0x0001;

    if (strcmp(ip, "0.0.0.0") != 0) {
        res->answers = malloc(sizeof(struct DnsOther));
        size_t answerNameLen = strlen(addr);
        res->answers->name = malloc(answerNameLen * sizeof(char) + 1);
        strcpy(res->answers->name, addr);
        res->answers->type = 0x0001;
        res->answers->class = 0x0001;
        res->answers->ttl = 400;
        res->answers->length = 4;
        res->answers->resource = malloc(sizeof(char) * 4);
        inet_ntop(AF_INET, &res->answers->resource, ip, INET_ADDRSTRLEN);
    }
    return res;
}


time_t getTTL(struct DnsPacket* packet) {
    time_t ttl = 1000;
    for (int i = 0; i < packet->header.answerCount; i++) {
        ttl = packet->answers[i].ttl < ttl ? packet->answers[i].ttl : ttl;
    }

    for (int i = 0; i < packet->header.authorityCount; i++) {
        ttl = packet->authorities[i].ttl < ttl ? packet->authorities[i].ttl : ttl;
    }

    for (int i = 0; i < packet->header.additionalCount; i++) {
        ttl = packet->additional[i].ttl < ttl ? packet->additional[i].ttl : ttl;
    }
    return ttl;
}