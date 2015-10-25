#include "receiver.h"
using namespace std;

void rcvchar(int sockfd, QTYPE *queue, int *j)
{

	int r; //result of sending signal to server
	if(queue->count > MIN_UPPER && !send_xoff){ // buffer full, sent xoff
		send_xon = false; //set xon flag
		send_xoff = true; //set xoff flag
		sent_xonxoff = XOFF; //assign signal

		buffer[0]=sent_xonxoff;

		//send xoff signal
		r = sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *)&cli_addr, sizeof(cli_addr));

		//check error on signal
		if(r>0)
			printf("Buffer > minimum upperlimit. Mengirim XOFF.\n");
		else
			printf("xoff signal failed\n");
	}

	//wait for next frame, put into frame_buffer
	r = recvfrom(sockfd, frame, FRAME_MAXLEN, 0, (struct sockaddr*) &cli_addr, (socklen_t*) &cli_len);

	//check recvfrom error
	if(r < 0){
		printf("error recvfrom\n");
	}

	// check end of file
	if(int(frame[3]) != Endfile){
		printf("Menerima pesan ke-%d.\n",*j);
	}
	else{
		*j=0; //reset j
		printf("End of file\n");
	}

	//update queue attr
	add(queue, frame);
}

/* q_get returns a pointer to the buffer where data is read or NULL if
* buffer is empty.
*/
void q_get(QTYPE *queue, FRAME *current)
{
	if (queue->count==0) {
		current=NULL;
	} else {
		del(queue, current);
	}

	//sending XON signal
	if(queue->count < MAX_LOWER && !send_xon){
		send_xon = true; //set xon flag
		send_xoff = false; //set xoff flag
		sent_xonxoff= XON;

		int r;
		buffer[0]=sent_xonxoff;
		r = sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *)&cli_addr, sizeof(cli_addr));

		//check error on signal
		if(r>0)
			printf("Buffer < maximum lowerlimit. Mengirim XON.\n");
		else
			printf("xon signal failed\n");
	}
}

// Thread function
void *consume(void *param){

	int i=1; //character index
	while (true) {

		/* Call q_get */
		FRAME res;
		q_get(rxq, &res);
		if(*res){
			printf("Error Checking byte ke-%d\n",i);
			i++;
			if(isValid(res,FRAME_MAXLEN)){
				if(res && (int(res[3])>32 || int(res[3])==CR || int(res[3])==LF || int(res[3])==Endfile )){
					//mark valid frame in recv_buffer
					markBuffer(res[1]);

					//send ack to transmitter
					sendACK(res[1]);

					//save frame to recv_buffer
					saveFrame(res);
				}
			}	else { // frame NAK
				printf("Frame is not valid\n");
			}
		}
		/* Can introduce some delay here. */
		usleep(DELAY); //delay
	}
	pthread_exit(0);
}

// ACK function
void sendACK(char bufferNUM){
  char ack[ACK_MAXLEN];

  ack[0]=ACK;
  ack[1]=bufferNUM;
  ack[2]=getCRC(ack, ACK_MAXLEN-1);
  sendto(sockfd, ack, ACK_MAXLEN,0,(struct sockaddr *)&cli_addr, sizeof(cli_addr));
  printf("Mengirim ACK untuk nomor buffer %c\n", bufferNUM);
}


void resetMarkBuffer(){
  for(int i=0; i<BUFFER_MAXLEN; i++){
    mark_buffer[i]=0;
  }
}


void markBuffer(char bufferNUM){
  mark_buffer[bufferNUM-'0']=1;
}

void *slideWindow(void *param){
  while(mark_buffer[WINDOW_START]){
    cout<<recv_buffer[WINDOW_START];
		mark_buffer[WINDOW_START]=0;
    WINDOW_START++;
    WINDOW_START%=BUFFER_MAXLEN;
    WINDOW_END++;
    WINDOW_END%=BUFFER_MAXLEN;
  }
}

// put ACKed frame to recv_buffer
void saveFrame(FRAME frame){
	int frame_number = frame[1];
	for(int i=0; i<FRAME_MAXLEN; ++i){
		recv_buffer[frame_number][i]= frame[i];
	}
	printf("add to received and valid frame buffer\n");
}

void add(QTYPE *queue, FRAME x){
	if(queue->front == (queue->rear+1)%(queue->maxsize))
	{
		printf("Circular Queue over flow");
	}
	else
	{
		if((queue->front ==0) && (queue->rear == 0))
		{
			queue->front=queue->rear+1;
		}
		queue->rear= (queue->rear+1)%(queue->maxsize);
		for(int i=0; i<FRAME_MAXLEN; ++i) {
			queue->data[queue->rear][i]=x[i];
		}
		queue->count++;
	}
}

void del(QTYPE *queue, FRAME *b){
	if(queue->front==0 && queue->rear == 0)
	{
		printf("Under flow");
	}
	else
	{
		for(int i=0; i<FRAME_MAXLEN; i++) {
			*b[i]=queue->data[queue->front][i];
		}
		queue->count--;
		if( queue->front == queue->rear )
		{
			queue->front=queue->rear=0;
		}
		else
		{
			queue->front= (queue->front+1)%(queue->maxsize);
		}
	}
}
