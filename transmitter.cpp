#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <iostream>
#include <regex.h>
#include <time.h>

#include "checksum.h"

#define SOH 1     /*Start of Header Character */
#define STX 2     /*Start of Text Character */
#define ETX 3     /*End of Text Character */
#define ACK 6     /*Acknowledgement */
#define NAK 21    /*Negative Acknowledgement */

/* Maximum messages length */
#define DATA_MAXLEN 2 // maximum length of data to be send
#define ACK_MAXLEN 3 // maximum length of ACK / NAK frame
#define FRAME_MAXLEN (5 + DATA_MAXLEN) // maximum length of data frame to be send
#define WINDOW_MAXLEN 5 // maximum number of possible sent frame
#define BUFFER_MAXLEN (WINDOW_MAXLEN * 2) // maximum number of frame buffer
#define FRAME_MAX_TIMEOUT 1 // maximum delay to receive ACK / NAK in seconds

int WINDOW_START = 0;
int WINDOW_END = (WINDOW_MAXLEN - 1) ;

#define XON (0x11)
#define XOFF (0x13)

int sockfd, port, n;
struct sockaddr_in serv_addr;
struct hostent *server;
FILE *fp;
bool is_ip_address;
char data_buffer[DATA_MAXLEN], sent_buffer[BUFFER_MAXLEN], frame_buffer[FRAME_MAXLEN], lastSignalRecv = XON;

char cache_buffer[BUFFER_MAXLEN][FRAME_MAXLEN];

time_t frameTimestamp[BUFFER_MAXLEN];
int ACK_array[BUFFER_MAXLEN];
int counter = 1;

int sent_frame = 0, last = 0;

using namespace std;

void prepareFrame(){
  frame_buffer[0] = SOH;
  frame_buffer[1] = '0' + last;
  frame_buffer[2] = STX;
  frame_buffer[3] = data_buffer[0];
  frame_buffer[4] = ETX;
  frame_buffer[5] = getCRC(frame_buffer, FRAME_MAXLEN - 2);

  printf("Frame Buffer to be sent : %x %x %x %x %x %x\n", frame_buffer[0], frame_buffer[1], frame_buffer[2], frame_buffer[3], frame_buffer[4], frame_buffer[5]);
}

void markFrameTimestamp(){
  int frameNumber = frame_buffer[1] - '0';
  time_t seconds = time (NULL);
  frameTimestamp[frameNumber] = seconds;
}

void printTimestamp(){
  printf("\n------------------\n");
  for (int i=0;i<FRAME_MAXLEN;i++){
    printf("Timestamp - %d : %ld\n", i, frameTimestamp[i]);
  }
  printf("------------------\n");
}

void printCacheBuffer(){
  printf("\n------------------\n");
  for (int i=0;i<WINDOW_MAXLEN;i++){
    printf("Cache Buffer for Frame - %d : '%x %x %x %x %x %x'\n", i, cache_buffer[i][0], cache_buffer[i][1], cache_buffer[i][2], cache_buffer[i][3], cache_buffer[i][4], cache_buffer[i][5]);
  }
  printf("------------------\n");
}

void removeFrameTimestamp(int frameNumber){
  frameTimestamp[frameNumber] = 0;
}

void remarkFrameTimestamp(int frameNumber){
  time_t seconds = time (NULL);
  frameTimestamp[frameNumber] = seconds;
}

void sendFrame(){
  markFrameTimestamp();
  // printTimestamp();

  int frameNumber = frame_buffer[1] - '0';
  memset(cache_buffer[frameNumber], '\0', sizeof(cache_buffer[frameNumber]));

  strcpy(cache_buffer[frameNumber], frame_buffer);

  // printCacheBuffer();

  printf("Frame - %d , Mengirim byte ke-%d: '%x'\n", frameNumber, counter, frame_buffer[3]);

  sendto(sockfd,frame_buffer, strlen(frame_buffer),0,(struct sockaddr *)&serv_addr,sizeof(serv_addr));
}

void sendFrameData(){
  prepareFrame();
  sendFrame();

  last++;
  last%=WINDOW_MAXLEN;
}

void resendFrame(int frameNumber){
  remarkFrameTimestamp(frameNumber);
  printf("Mengirim ulang frame ke-%d: '%x %x %x %x %x %x'\n", frameNumber, cache_buffer[frameNumber][0], cache_buffer[frameNumber][1], cache_buffer[frameNumber][2], cache_buffer[frameNumber][3], cache_buffer[frameNumber][4], cache_buffer[frameNumber][5]);
  sendto(sockfd,cache_buffer[frameNumber], strlen(cache_buffer[frameNumber]),0,(struct sockaddr *)&serv_addr,sizeof(serv_addr));
}

bool isError(char* ack_frame){
  return true;
}

void markACK(char frameNumber){
  ACK_array[frameNumber - '0'] = 1;
}

void moveWindow(){
  printf("WINDOW_START : %d\n", WINDOW_START);
  printf("WINDOW_END : %d\n", WINDOW_END);
  while (ACK_array[WINDOW_START]){
    ACK_array[WINDOW_START] = 0;
    WINDOW_START++;
    WINDOW_START%=BUFFER_MAXLEN;
    WINDOW_END++;
    WINDOW_END%=BUFFER_MAXLEN;
    printf("before sent_frame: %d\n", sent_frame);
    sent_frame--;
    printf("after sent_frame: %d\n", sent_frame);
  }
}

void prepareSocket(int argc, char *argv[]){
  port = atoi(argv[2]);

  /* Create a socket point */
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);

  if (sockfd < 0) {
      perror("ERROR opening socket");
      exit(1);
  }

  /* Empty buffer */
  bzero((char *) &serv_addr, sizeof(serv_addr));

  serv_addr.sin_family = AF_INET;

  int r;
  regex_t reg;

  if (r = regcomp(&reg, "^(\\d{0,3}\\.){3}\\d{0,3}$", REG_NOSUB | REG_EXTENDED) ){
      char errbuf[1024];
      regerror(r, &reg, errbuf, sizeof(errbuf));
      printf("error: %s\n", errbuf);
      exit(1);
  }

  if (regexec(&reg, argv[1],0,NULL,0) != REG_NOMATCH){
      is_ip_address = true;
  }

  if ( is_ip_address ) {
      serv_addr.sin_addr.s_addr = inet_addr(argv[1]);

      printf("Membuat socket untuk koneksi ke %s:%s ...", argv[1], argv[2]);
  } else {
      /* Map host name to ip address */
      server = gethostbyname(argv[1]);

      if (!server) {
         fprintf(stderr,"ERROR, no such host\n");
         exit(0);
      }

      bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
  }

  /* converts a port number in host byte order to a port number in network byte order */
  serv_addr.sin_port = htons(port);
}

static void *receiveSignal(void*);
static void *timerTimeout(void*);

int main(int argc, char *argv[]){

  printf("Hello I am main thread \n");

  if (argc < 4) {
      fprintf(stderr,"Usage : %s hostname port filename\n", argv[0]);
      exit(0);
  }

  prepareSocket(argc, argv);

  /* Create new thread */
  pthread_t receive_signal_thread;
  pthread_t timer_timeout_thread;

  // "CONSUME" THREAD
  if(pthread_create(&receive_signal_thread, NULL, &receiveSignal, NULL)){
      fprintf(stderr, "Error creating thread\n");
      return 1;
  }

  // "CONSUME" THREAD
  if(pthread_create(&timer_timeout_thread, NULL, &timerTimeout, NULL)){
      fprintf(stderr, "Error creating thread\n");
      return 1;
  }

  printf("Opening file %s \n",argv[3]);

  /* opening file for reading */
  fp = fopen(argv[3] , "r");
  if(fp == NULL)
  {
     perror("Error opening file");
     exit(1);
  }

  memset(frameTimestamp, '\0', sizeof(frameTimestamp));

  while (true){
    // printf("sent_frame : %d\n", sent_frame);
    if (sent_frame < WINDOW_MAXLEN){
      if (fgets(data_buffer, DATA_MAXLEN, fp) != NULL ) {
        bool xx = false;
        while (lastSignalRecv == XOFF) {
          if (!xx){
              printf("Menunggu XON...\n");
              xx = true;
          }
        }

        sendFrameData();
        sent_frame++;

        bzero(data_buffer, DATA_MAXLEN);
        counter++;
      } else {
        printf("Exiting parent...\n");
        return 0;
      }
    }
  }

  printf("Exiting parent...\n");
  return 0;
}

// Thread function
static void *receiveSignal(void* param){
  printf("Hello I am signal receiver thread\n");

  while (true) {
    int serv_len = sizeof(serv_addr);

    char _buffer[ACK_MAXLEN];

    n = recvfrom(sockfd,_buffer,strlen(_buffer),0,(struct sockaddr *)&serv_addr,(socklen_t*) &serv_len);
    // buffer[n]=0;
    // fputs(_buffer,stdout);

    if (n < 0) {
       perror("ERROR reading from socket");
       exit(1);
    }

    lastSignalRecv = _buffer[0];

    if (lastSignalRecv == XOFF) {
        printf("XOFF diterima.\n");
    } else if (lastSignalRecv == XON) {
        printf("XON diterima.\n");
    } else if (lastSignalRecv == ACK) {
        if (isValid(_buffer, ACK_MAXLEN)){
          int frameNumber = _buffer[1] - '0';
          printf("ACK diterima dengan nomor frame %d.\n", frameNumber);
          markACK(frameNumber + '0');
          removeFrameTimestamp(frameNumber);
          moveWindow();
        }
    } else if (lastSignalRecv == NAK) {
        if (isValid(_buffer, ACK_MAXLEN)){
          printf("NAK diterima dengan nomor frame %d.\n", _buffer[1]);
          resendFrame(_buffer[1] - '0');
        }
    }
  }
  printf("Exiting child...\n");
  pthread_exit(0);
}

// Thread function
static void *timerTimeout(void* param){
  printf("Hello I am timer thread\n");

  while (true) {
    for (int i=WINDOW_START;i<=WINDOW_END;i++){
      time_t now = time (NULL);

      if ((frameTimestamp[i] != 0) && (now - frameTimestamp[i] > FRAME_MAX_TIMEOUT)){
        resendFrame(i);
      }
    }
  }

  printf("Exiting child...\n");
  pthread_exit(0);
}
