#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <ctime>
#include <sstream>
#include <signal.h>
#include <iostream>
#include <vector>
#include <math.h>    
#include <algorithm>    
#include <iterator>  
using namespace std;

#define MAX_PROPOSAL_VAL 9
#define MAXBUFLEN 100
#define NUM_LEARNERS 1
#define NUM_ACCEPTORS 3
#define PHASE_PREPARE "p"
#define PHASE_ACCEPT "a"

const char* learnerPorts[] = {"5064"};

struct Proposal {
	string phase;  
    int number;   
    int value;  
}; 

int bindSocket(const char* port, addrinfo *&p){
	int sockfd, rv;
	struct addrinfo hints, *servinfo;
	memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; 
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE; 

	if ((rv = getaddrinfo(NULL, port, &hints, &servinfo)) != 0) {
		perror("error1");
       	exit(1);
    }
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            continue;
        }
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            continue;
        }
        break;
    }
    if (p == NULL) {
    	perror("error1");
        exit(1);
    }

    freeaddrinfo(servinfo);
    //printf("Returning socket %d on port %s\n", sockfd, acceptorPort);
    return sockfd;
}

int main(int argc, char *argv[]){

	int num_learners = NUM_LEARNERS;
	stringstream argstream;
	if (argc == 4){
		argstream << argv[3];
		argstream >> num_learners;
	}

	//fork off the acceptors
	for (int i = 0; i < num_learners; i++){
		if (!fork()){
			//listen
			struct addrinfo *myp;
			socklen_t addr_len, to;
			struct sockaddr_storage their_addr;
   			char buf[MAXBUFLEN];
			int receiveBytes, sentBytes, socket;
			socket = bindSocket(learnerPorts[i], myp);
			//printf("Listening to socket %d on port %s\n", receiveSocket, acceptorPorts[i]);

			//infinitely receive prepares/accepts and reply accordingly
			printf("L%d: Waiting for proposals\n", i);
			int numProposalsReceived = 0;
			while (1) {
				Proposal proposal;
				addr_len = sizeof(their_addr);
				if ((receiveBytes = recvfrom(socket, buf, MAXBUFLEN-1 , 0, (struct sockaddr *)&their_addr, &addr_len)) == -1) {
	        		printf("error\n");
	        		exit(1);
	    		}
	    		if (receiveBytes == 0) {
	    			continue;
	    		}
	    		printf("L%d: Received proposal %s\n", i, buf);
	    		numProposalsReceived++;
	    		if (numProposalsReceived == NUM_ACCEPTORS){
	    			printf("The end.\n");
	    			exit(0);
	    		}

	    	}
	    }
	}
	while(1){}
}
