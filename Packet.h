#ifndef PACKET_H
#define PACKET_H

#include <stdint.h>

struct PACKET {
    uint32_t opcode;
    char login[20];
    unsigned char hash[64];
};

struct RESPONSE {
    uint32_t opcode;
};

#endif // PACKET_H
