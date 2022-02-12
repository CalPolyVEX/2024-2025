/*
File: PacketCRC.h
Description: This file contains definitions for executing CRC-16 Checksum
of data packets
Last Updated: 2/12/22
*/

#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

typedef uint8_t byte;

typedef struct {
    uint8_t poly;
    uint8_t crcLen;
    uint8_t tableLen; 
    uint8_t *csTable;
} PacketCRC;

PacketCRC crc_init(const uint8_t polynomial, const uint8_t crcLen);

void freeTable(PacketCRC crc);

void generateTable(PacketCRC crc);

uint8_t calculate_test(PacketCRC crc, const uint8_t val);

uint8_t calculate(PacketCRC pack, uint8_t arr[], uint8_t len);

