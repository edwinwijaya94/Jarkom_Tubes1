#include "receiver.h"
#include <pthread.h>
#include <unistd.h>

using namespace std;

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
	pthread_t sliding_thread;

	// "CONSUME" THREAD
	rc = pthread_create(&consume_thread, NULL, &consume, NULL);
		if (rc) {
			printf("ERROR; return code from pthread_create() is %d\n", rc);
			exit(-1);
		}

	sw = pthread_create(&sliding_thread, NULL, &slideWindow, NULL);
		if (sw) {
			printf("ERROR; return code from pthread_create() is %d\n", rc);
			exit(-1);
		}


	// 'RECEIVE' THREAD
	resetMarkBuffer();

	int j=1;
	while (true) {
		rcvchar(sockfd, &rcvq, &j);
		//delay
		// usleep(300000);
		j++;
	}

	//join thread
	if(pthread_join(consume_thread,NULL)){
		fprintf(stderr,"Error joining thread\n");
		return 2;
	}

  if(pthread_join(sliding_thread,NULL)){
    fprintf(stderr,"Error joining thread\n");
    return 2;
  }

	return 0;
}
