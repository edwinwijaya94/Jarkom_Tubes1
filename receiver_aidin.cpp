#include "receiver.h"
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

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
void *q_get(void *param)
{
	while(true){
		slideWindow();

		//sending XON signal
		if(buffer.count < BUFFER_MAXLEN && !send_xon){
			send_xon = true; //set xon flag
			send_xoff = false; //set xoff flag
			sent_xonxoff= XON;

			int r;
			signal_buffer[0]=sent_xonxoff;
			r = sendto(sockfd, signal_buffer, strlen(signal_buffer), 0, (struct sockaddr *)&cli_addr, sizeof(cli_addr));

			//check error on signal
			if(r>0)
				printf("Mengirim XON.\n");
			else
				printf("xon signal failed\n");

		}
	}
}

// Thread function
void *consume(void *param){
	//int i=1; //character index
	while (true) {
		//slideWindow(); // print ACKed frame and slide window to right
		while(temp_count){
		// check is frame valid
			FRAME temp_frame;
			strcpy(temp_frame,temp_buffer[check_index]);
			check_index  = (check_index +1) % BUFFER_MAXLEN;
			temp_count--;

			if(isValid(temp_frame,FRAME_MAXLEN)){

				printf("Frame ke-%d valid\n",temp_frame[1]-'0');

				//send ack to transmitter
				// sendACK(frame[1]);
				char ack[ACK_MAXLEN];

				ack[0]=ACK;
				ack[1]=temp_frame[1];
				ack[2]=getCRC(ack,2);

				printf("ack : %x\n", ack);

				ssize_t temp = sendto(sockfd, ack, ACK_MAXLEN,0,(struct sockaddr *)&cli_addr, sizeof(cli_addr));
				printf("sendto : %d\n", temp);

				char msg_buff[100];

				strerror_r( errno, msg_buff, 100 );
				printf("error message : %s\n", msg_buff);

				printf("Mengirim ACK untuk nomor frame %c\n", temp_frame[1]);

				//add frame to buffer.data
				if(buffer.data[0+temp_frame[1]][3]!=temp_frame[3]){
					add(temp_frame);
				}
			}
			else { // frame NAK
				printf("Frame ke-%d tidak valid !\n");
			}
		}

		//usleep(DELAY); //delay
	}
	pthread_exit(0);
}

// ACK function
/*void sendACK(char bufferNUM){

}*/


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
  if(buffer.mark_buffer[buffer.WINDOW_START]){
		printf("windowstart=%d\n",buffer.WINDOW_START);
    printf("windowend=%d\n",buffer.WINDOW_END);
		cout<<"isi frame ke-"<<buffer.WINDOW_START<<" = "<<buffer.data[buffer.WINDOW_START][3]<<endl;
		buffer.mark_buffer[buffer.WINDOW_START]=0;
    buffer.WINDOW_START++;
    buffer.WINDOW_START%=BUFFER_MAXLEN;
    buffer.WINDOW_END++;
    buffer.WINDOW_END%=BUFFER_MAXLEN;
  	buffer.count--;
  	printf("count after slides :%d\n",buffer.count);
  }
}


void add(FRAME x){

	if(buffer.count == BUFFER_MAXLEN)
	{
		printf("Circular BUFFER full\n");
	}
	else
	{
		printf("add as valid frame to buffer\n");
		printf("add_windowstart=%d\n",buffer.WINDOW_START);
		if((buffer.WINDOW_START ==0) && (buffer.WINDOW_END == 0))
		{
			buffer.WINDOW_START=buffer.WINDOW_END+1;
		}

		int frame_number = x[1]-'0';
		for(int i=0; i<FRAME_MAXLEN; i++) {
			buffer.data[(buffer.WINDOW_START + frame_number) % BUFFER_MAXLEN][i]=x[i];
		}

		//mark valid frame in buffer
		markBuffer(x[1]);
		printf("after mark buffer\n");
		for(int j=0; j<BUFFER_MAXLEN; j++)
			printf("%d",buffer.mark_buffer[j]);
		printf("\n");

		buffer.count++;
		printf("count after add = %d\n",buffer.count);
		printf("after add:\n");
		for(int i=0; i<BUFFER_MAXLEN; i++){
			printf("%c",buffer.data[i][3]);
		}
		printf("\n");

	}
}

int main(int argc, char const *argv[]) {
  /* code */
	FRAME c;
	int rc;
	int sw;

	//create socket
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(sockfd <0)
		printf("error socket\n");

	//Insert code here to bind socket to the port number given in argv[1].
	memset((char *)&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET; //assign as UDP
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); //assign server address
	serv_addr.sin_port = htons(atoi(argv[1])); //assign port number

	//binding
	if(bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))< 0)
		perror("error on binding\n");
	else
		printf("Binding pada port: %d\n",atoi(argv[1]));

	/* Initialize XON/XOFF flags */
	sent_xonxoff = XON;

	/* Create new thread */
	pthread_t consume_thread;
	pthread_t slide_thread;

	// "CONSUME" THREAD
	if (pthread_create(&consume_thread, NULL, consume, &buffer)) {
		printf("ERROR; return code from pthread_create() is %d\n", rc);
		exit(-1);
	}

	// "SLIDE WINDOW" THREAD
	if (pthread_create(&slide_thread, NULL, q_get, &buffer)) {
		printf("ERROR; return code from pthread_create() is %d\n", rc);
		exit(-1);
	}

	// 'RECEIVE' THREAD
	resetMarkBuffer();

	int j=0;
	while (true) {
		rcvchar(sockfd, &buffer, &j);
		//delay
		// usleep(300000);
		j++;
	}

	//join thread
	if(pthread_join(consume_thread,NULL)){
		fprintf(stderr,"Error joining thread\n");
		return 2;
	}

	if(pthread_join(slide_thread,NULL)){
		fprintf(stderr,"Error joining thread\n");
		return 2;
	}

	return 0;
}
