
#ifndef receiver_H
#define receiver_H
/*
* File : T1_rx.cpp
*/
#include "checksum.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <netdb.h>
#include <pthread.h>

/* Delay to adjust speed of consuming buffer, in microseconds */
#define DELAY 100000
#define RXQSIZE 10
#define MIN_UPPER 2
#define MAX_LOWER 2
#define DATA_MAXLEN 2 // maximum length of data to be send
#define ACK_MAXLEN 3 // maximum length of ACK / NAK frame
#define FRAME_MAXLEN (5 + DATA_MAXLEN) // maximum length of data frame to be received
#define WINDOW_MAXLEN 5 // maximum number of possible received frame
#define BUFFER_MAXLEN (WINDOW_MAXLEN * 2) // maximum number of frame buffer
#define SOH 1 /* Start of Header Character */
#define STX 2 /* Start of Text Character */
#define ETX 3 /* End of Text Chacracter */
#define ENQ 5 /* Enquiry Character */
#define ACK 6 /* Acknowledgement */
#define BEL 7 /* Message Error Warning */
#define CR 13 /* Carriage Return */
#define LF 10 /* Line Feed */
#define NAK 21 /* Negative Acknowledgement */
#define Endfile 26 /* End of File Character */
#define ESC 27 /* ESC key */
#define XON (0x11)
#define XOFF (0x13)
using namespace std;




static char signal_buffer[2]; //for sending XON/XOFF signal
static struct sockaddr_in serv_addr; //sockaddr for server
static struct sockaddr_in cli_addr; //sockaddr for client
static int cli_len= sizeof(cli_addr); // sizeof client address

typedef char FRAME[FRAME_MAXLEN];

typedef struct BUFFER
{
	unsigned int WINDOW_START;
	unsigned int WINDOW_END;
	unsigned int *mark_buffer;
	unsigned int count;
	unsigned int maxsize;
	FRAME *data;
} BUFFER;

static unsigned int marks[BUFFER_MAXLEN];
static FRAME recv_buffer[RXQSIZE];
static BUFFER buffer = { 0, 4, marks, 0, BUFFER_MAXLEN, recv_buffer };
static unsigned char sent_xonxoff = XON;
static bool send_xon = true, send_xoff = false;

static FRAME temp_buffer[BUFFER_MAXLEN];

static FRAME frame;
/* Socket */
static int sockfd; // listen on sock_fd

/* Functions declaration */
void add(FRAME x);
void del(BUFFER *buffer, FRAME *b);
//void sendACK(char bufferNUM);
void markBuffer(char bufferNUM);
void resetMarkBuffer();
//void saveFrame(char*);
void rcvchar(int sockfd, BUFFER *buffer, int *j);
void q_get(BUFFER *buffer, FRAME *current);
void *consume(void *param);
void slideWindow();


#endif
