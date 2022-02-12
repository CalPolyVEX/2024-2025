/*
File: PacketCRC.c
Description: This file contains functions for executing CRC-16 Checksum
of data packets
Last Updated: 2/12/22
*/
#include "PacketCRC.h"

PacketCRC crc_init(const uint8_t polynomial, const uint8_t crcLen) {
    PacketCRC crc;
    crc.poly = polynomial;
    crc.crcLen = crcLen;
    crc.tableLen = pow(2, crcLen);
    if(!(crc.csTable = malloc(sizeof(uint8_t) * crc.tableLen))) {
        perror("crc");
        exit(EXIT_FAILURE);
    }
    generateTable(crc);
    return crc;
}

void freeTable(PacketCRC crc){
    free(crc.csTable);
}

void generateTable(PacketCRC crc) {
    for (uint16_t i = 0; i < crc.tableLen; ++i) {
        int curr = i;

        for (int j = 0; j < 8; ++j) {
            if ((curr & 0x80) != 0)
                curr = (curr << 1) ^ (int)(crc.poly);
            else
                curr <<= 1;
        }

        crc.csTable[i] = (byte)curr;
    }
}

uint8_t calculate_test(PacketCRC crc, const uint8_t val) {
    if (val < crc.tableLen)
        return crc.csTable[val];
    return 0;
}

uint8_t calculate(PacketCRC pack, uint8_t arr[], uint8_t len) {
    uint8_t crc = 0;
    for (uint16_t i = 0; i < len; i++)
        crc = pack.csTable[crc ^ arr[i]];

    return crc;
}
