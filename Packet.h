#ifndef PACKET_H
#define PACKET_H

#include <stdint.h>

typedef struct {
    uint32_t opcode;
    char login[20];
    unsigned char hash[64];
} Packet;

typedef struct {
    uint32_t opcode;
    char name[13];
} GamePacket;

typedef struct {
    uint32_t opcode;
} Response;

#endif // PACKET_H
