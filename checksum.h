#ifndef _CHECKSUM_H_
#define _CHECKSUM_H_

// CRC polynomial
#define CRC8 0xD5

// MAX LONG
#define MAXLONG 0xFFFFFFFF // max unsigned long

//get CRC value
unsigned char getCRC(unsigned char* message, int length);

// check if a message contains CRC is valid
bool isValid(unsigned char* message, int size);

#endif
