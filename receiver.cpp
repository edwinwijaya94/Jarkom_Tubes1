#include "receiver.h"
using namespace std;

int temp_index = 0;
int check_index = 0;
int temp_count = 0;

void rcvchar(int sockfd, BUFFER *buffer, int *j)
{

	int r; //result of sending signal to server
	if(temp_count == BUFFER_MAXLEN && !send_xoff){ // buffer full, sent xoff
		send_xon = false; //set xon flag
		send_xoff = true; //set xoff flag
		sent_xonxoff = XOFF; //assign signal

		signal_buffer[0]=sent_xonxoff;

		//send xoff signal
		r = sendto(sockfd, signal_buffer, strlen(signal_buffer), 0, (struct sockaddr *)&cli_addr, sizeof(cli_addr));

		//check error on signal
		if(r>0)
			printf("Buffer > minimum upperlimit. Mengirim XOFF.\n");
		else
			printf("xoff signal failed\n");
	}

	//wait for next frame, put into frame_buffer
	r = recvfrom(sockfd, frame, FRAME_MAXLEN, 0, (struct sockaddr*) &cli_addr, (socklen_t*) &cli_len);

	strcpy(temp_buffer[temp_index], frame);
	temp_index = (temp_index+1)%BUFFER_MAXLEN;
	temp_count++;
	
	//check recvfrom error
	if(r < 0){
		printf("error recvfrom\n");
	}

	// check end of file
	if(int(frame[3]) != Endfile){
		printf("Menerima frame ke-%d.\n",*j);

		
	}
	else{
		*j=0; //reset j
		printf("End of file received !\n");
	}
}

/* q_get returns a pointer to the buffer where data is read or NULL if
* buffer is empty.
*/
void q_get(BUFFER *buffer, FRAME *current)
{
	if (buffer->count == 0) {
		current=NULL;
	} else {
		//del(buffer, current);
		for(int i=0; i<FRAME_MAXLEN; i++)
			*current[i] = buffer->data[buffer->WINDOW_START][i];
	}

	//sending XON signal
	if(buffer->count < MAX_LOWER && !send_xon){
		send_xon = true; //set xon flag
		send_xoff = false; //set xoff flag
		sent_xonxoff= XON;

		int r;
		signal_buffer[0]=sent_xonxoff;
		r = sendto(sockfd, signal_buffer, strlen(signal_buffer), 0, (struct sockaddr *)&cli_addr, sizeof(cli_addr));

		//check error on signal
		if(r>0)
			printf("Buffer < maximum lowerlimit. Mengirim XON.\n");
		else
			printf("xon signal failed\n");

	}
}

// Thread function
void *consume(void *param){
	printf("Consume called !\n");
	int i=1; //character index
	while (true) {
		while(temp_count){
		// check is frame valid
			FRAME temp_frame;
			strcpy(temp_frame,temp_buffer[check_index]); 
			if(isValid(temp_frame,FRAME_MAXLEN)){
				temp_count--;
				printf("Frame ke-%d valid\n",temp_frame[1]-'0');
				//mark valid frame in buffer
				markBuffer(temp_frame[1]);

				//send ack to transmitter
				// sendACK(frame[1]);
				char ack[ACK_MAXLEN];

				ack[0]=ACK;
				ack[1]=temp_frame[1];
				ack[2]=getCRC(ack, ACK_MAXLEN-1);

				printf("ack : %x\n", ack);

				sendto(sockfd, ack, sizeof ack,0,(struct sockaddr *)&cli_addr, sizeof(cli_addr));
				printf("Mengirim ACK untuk nomor buffer %c\n", temp_frame[1]);

				//add frame to buffer.data
				add(temp_frame);
			}	
			else { // frame NAK
				temp_count--;
				printf("Frame ke-%d tidak valid !\n");
			}
			check_index = (check_index+1) %BUFFER_MAXLEN;  
		}

		/* Call q_get */
		//FRAME res;
		//q_get(&buffer, &res);
		slideWindow(); // print ACKed frame and slide window to right
		/*if(*res){
			//printf("");
			i++;
		}
		else{
			//printf("res NULL\n");
		}*/
		/* Can introduce some delay here. */
		usleep(DELAY); //delay
	}
	pthread_exit(0);
}

// ACK function
void sendACK(char bufferNUM){
  
}


void resetMarkBuffer(){
  for(int i=0; i<BUFFER_MAXLEN; i++){
    buffer.mark_buffer[i]=0;
  }
}


void markBuffer(char bufferNUM){
	//printf("markbuffer\n");
	buffer.mark_buffer[bufferNUM-'0']=1;
}

void slideWindow(){
	
  while(buffer.mark_buffer[buffer.WINDOW_START]){
    //printf("windowstart=%d\n",buffer.WINDOW_START);
	cout<<"isi frame ke-"<<buffer.WINDOW_START<<" = "<<buffer.data[buffer.WINDOW_START][3]<<endl;	
	buffer.mark_buffer[buffer.WINDOW_START]=0;
    buffer.WINDOW_START++;
    buffer.WINDOW_START%=BUFFER_MAXLEN;
    buffer.WINDOW_END++;
    buffer.WINDOW_END%=BUFFER_MAXLEN;
  	buffer.count--;
  }
}

// put ACKed frame to recv_buffer
/*void saveFrame(FRAME frame){
	int frame_number = frame[1];
	for(int i=0; i<FRAME_MAXLEN; ++i){
		buffer.data[frame_number][i]= frame[i];
	}
	printf("add to received and valid frame buffer\n");
}*/

void add(FRAME x){
	
	/*printf("add as valid frame to buffer\n");
	if((buffer.WINDOW_START ==0) && (buffer.WINDOW_END == 0))
	{
		buffer.WINDOW_START=buffer.WINDOW_END+1;
	}
	
	int frame_number = x[1]-'0';
	for(int i=0; i<FRAME_MAXLEN; i++) {
		buffer.data[frame_number][i]=x[i];
	}
	buffer.count++;
	
	buffer.WINDOW_END= (buffer.WINDOW_END+1)%(buffer.maxsize);*/
		
	if(buffer.WINDOW_START == (buffer.WINDOW_END+1)%(buffer.maxsize))
	{
		printf("Circular BUFFER over flow\n");
	}
	else
	{
		printf("add as valid frame to buffer\n");
		if((buffer.WINDOW_START ==0) && (buffer.WINDOW_END == 0))
		{
			buffer.WINDOW_START=buffer.WINDOW_END+1;
		}
		
		int frame_number = x[1]-'0';
		for(int i=0; i<FRAME_MAXLEN; i++) {
			buffer.data[frame_number][i]=x[i];
		}
		buffer.count++;
		
		buffer.WINDOW_END= (buffer.WINDOW_END+1)%(buffer.maxsize);
	}
}

void del(BUFFER *buffer, FRAME *b){
	if(buffer->WINDOW_START==0 && buffer->WINDOW_END == 0)
	{
		printf("Under flow\n");
	}
	else
	{
		for(int i=0; i<FRAME_MAXLEN; i++) {
			*b[i]=buffer->data[buffer->WINDOW_START][i];
		}
		buffer->count--;
		if( buffer->WINDOW_START == buffer->WINDOW_END )
		{
			buffer->WINDOW_START=buffer->WINDOW_END=0;
		}
		else
		{
			buffer->WINDOW_START= (buffer->WINDOW_START+1)%(buffer->maxsize);
		}
	}
}
