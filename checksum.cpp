/* IMPLEMENTATIONS OF CHECKSUM FUNCTION USING CRC METHOD */

#include <iostream>
#include <stdint.h>
#include <cstdio>

#include "checksum.h"
 
using namespace std;

//get CRC value
unsigned char getCRC(unsigned char* message, int length)
{
  int i, j;
  unsigned char crc = 0;
 
  for (i = 0; i < length; i++)
  {
    crc ^= message[i];
    for (j = 0; j < 8; j++)
    {
      if (crc & 1)
        crc ^= CRC8;
      crc >>= 1;
    }
  }
  return crc;
}

// check if a message contains CRC is valid
bool isValid(unsigned char* message, int size){
	
	if (getCRC(message, size))
		return false;
	else
		return true;
}

/*void dec2bin(int c)
{
   int i = 0;
   for(i = 31; i >= 0; i--){
     if((c & (1 << i)) != 0){
       printf("1");
     }else{
       printf("0");
     } 
   }
}

int main(){
	unsigned char message[3] = {0x83, 0x01, 0x00};
	message[2] = getCRC(message, 2);
	return 0;
}*/
