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
#define MAXBUFLEN 5
#define NUM_ACCEPTORS 3
#define NUM_LEARNERS 1
#define PHASE_PREPARE "p"
#define PHASE_ACCEPT "a"
#define IGNORE "ignor"
#define ACCEPT "accep"

const char* proposerPorts[] = {"9000", "9001", "9002"};
const char* acceptorPorts[] = {"4064", "4164", "4264"};
const char* learnerPorts[] = {"5064"};

struct Proposal {
	string phase;  
    int number;   
    int value;  
}; 

Proposal messageToProposal(char buf[]){
	Proposal p;
  	char* token;
  	token = strtok (buf,":");
  	int counter = 0;
  	while (token != NULL) {
  		istringstream ss(token);
	 	if (counter == 0){
  			ss >> p.phase;
  			//printf("phase is %s\n", p.phase.c_str());
  		} else if (counter == 1){
  			ss >> p.number;
  			//printf("number is %d\n", p.number);
  		} else if (counter == 2){
  			ss >> p.value;
  			//printf("value is %d\n", p.value);
  		}
  		counter++;
    	token = strtok (NULL, ":");
  	}
  	return p;
}

string proposalToMessage(Proposal p){
	string message;
	stringstream ss;
	ss << p.phase << ":" << p.number << ":" << p.value;
	ss >> message;
	return message;
}

int bindSocket(const char* acceptorPort, addrinfo *&p){
	int sockfd, rv;
	struct addrinfo hints, *servinfo;
	memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; 
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE; 

	if ((rv = getaddrinfo(NULL, acceptorPort, &hints, &servinfo)) != 0) {
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
        exit(1);
    }

    freeaddrinfo(servinfo);
    //printf("Returning socket %d on port %s\n", sockfd, acceptorPort);
    return sockfd;
}

int getSocket(string portString, addrinfo *&p){
	int sockfd, rv;
    struct addrinfo hints, *servinfo;
    struct hostent *h;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;

    char hostname[1024];
    hostname[1023] = '\0';
    gethostname(hostname, 1024);
    h = gethostbyname(hostname);
    char* ip = inet_ntoa(*((struct in_addr *)h->h_addr));

    const char* port = portString.c_str();
    if ((rv = getaddrinfo(ip, port, &hints, &servinfo)) != 0) {
        exit(1);
   	}
    for(p = servinfo; p != NULL; p = p->ai_next) {
    	if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
        	continue;
        }
        break;
    }
    if (p == NULL) {
        exit(1);		
    }
    //printf("Returning socket %d on port %s\n", sockfd, port);
    return sockfd;
}

int main(int argc, char *argv[]){

	int num_acceptors = NUM_ACCEPTORS;
	int num_learners = NUM_LEARNERS;
	stringstream argstream;
	if (argc == 4){
		argstream << argv[2];
		argstream >> num_acceptors;
		argstream.clear();
		argstream << argv[3];
		argstream >> num_learners;
	}

	//fork off the acceptors
	for (int i = 0; i < num_acceptors; i++){
		if (!fork()){

			Proposal highestProposalRespondedTo;
			Proposal highestProposalAccepted;

			//listen
			struct addrinfo *myp;
			socklen_t addr_len, to;
			struct sockaddr_storage their_addr;
   			char buf[MAXBUFLEN];
			int receiveBytes, sentBytes, socket;
			socket = bindSocket(acceptorPorts[i], myp);
			//printf("Listening to socket %d on port %s\n", receiveSocket, acceptorPorts[i]);

			//infinitely receive prepares/accepts and reply accordingly
			while (1) {
				Proposal proposal;
				printf("A%d: Waiting for proposals on %s\n", i, acceptorPorts[i]);
				addr_len = sizeof(their_addr);
				if ((receiveBytes = recvfrom(socket, buf, MAXBUFLEN , 0, (struct sockaddr *)&their_addr, &addr_len)) == -1) {
	        		printf("error\n");
	        		exit(1);
	    		}
	    		if (receiveBytes == 0) {
	    			printf("Received nothing.\n");
	    			continue;
	    		}
	    		printf("A%d: Received proposal %s\n", i, buf);

	    		proposal = messageToProposal(buf);
	    		if (proposal.phase == PHASE_PREPARE){
	    			//printf("A%d: prepare message\n", i);
	    			bool respond = false;
					if (highestProposalRespondedTo.number == 0){
						respond = true;
					} else {
						if (proposal.number >= highestProposalRespondedTo.number){
							respond = true;
						} 
					}
					if (respond){
						//respond with highestproposalrespondedto and then update it
						string message;
						if (highestProposalRespondedTo.number == 0){
							message = "none";
						} else {
    					 	message = proposalToMessage(highestProposalRespondedTo);
    					}
    			 		int n = message.length(); 
     			    	char char_array[n];
     			    	char_array[n-1] = '\0'; 
    					strcpy(char_array, message.c_str()); 
    					if ((sentBytes = sendto(socket, char_array, sizeof(char_array), 0, (struct sockaddr *)&their_addr, addr_len)) == -1) {
	        				perror("error: sending");
        					exit(1);
    					}
    					printf("A%d: Sent response message %s to proposer\n", i, message.c_str());
						highestProposalRespondedTo.phase = proposal.phase;
						highestProposalRespondedTo.number = proposal.number;
						highestProposalRespondedTo.value = proposal.value;
						//printf("A%d: highest proposal responded to: %s:%d:%d\n", i, highestProposalRespondedTo.phase.c_str(), highestProposalRespondedTo.number, highestProposalRespondedTo.value);
					} else {
						string m = IGNORE;
						int n = m.length(); 
     			    	char char_array[n];
     			    	char_array[n-1] = '\0'; 
    					strcpy(char_array, m.c_str()); 
						if ((sentBytes = sendto(socket, char_array, sizeof(char_array), 0, (struct sockaddr *)&their_addr, addr_len)) == -1) {
	        				perror("error: sending");
        					exit(1);
    					}
    					printf("A%d: Sent response message %s to proposer\n", i, m.c_str());
					}

				} else if (proposal.phase == PHASE_ACCEPT) {
					//printf("A%d: accept message\n", i);
					//printf("A%d: current phase: %s, number: %d, value: %d\n", i, proposal.phase.c_str(), proposal.number, proposal.value);
					//printf("A%d: highest phase: %s, number: %d, value: %d\n", i, highestProposalRespondedTo.phase.c_str(), highestProposalRespondedTo.number, highestProposalRespondedTo.value);
					if (proposal.number >= highestProposalRespondedTo.number){
						printf("A%d: Accepted proposal %d:%d\n", i, proposal.number, proposal.value);

						//notify proposer
						string m = ACCEPT;
						int n = m.length(); 
     			    	char char_array[n];
     			    	char_array[n-1] = '\0'; 
    					strcpy(char_array, m.c_str()); 
						if ((sentBytes = sendto(socket, char_array, sizeof(char_array), 0, (struct sockaddr *)&their_addr, addr_len)) == -1) {
	        				perror("error: sending");
        					exit(1);
    					}
    					printf("A%d: Sent response message %s to proposer\n", i, char_array);

    					//notify learner
    					string message = proposalToMessage(proposal);
    					n = message.length(); 
     			    	char_array[n];
     			    	char_array[n-1] = '\0'; 
    					strcpy(char_array, message.c_str()); 
    					for (int l = 0; l < num_learners; l++){
    						struct addrinfo *theirp;
							int sentBytes, sendSocket;
							sendSocket = getSocket(learnerPorts[l], theirp);
    						if ((sentBytes = sendto(sendSocket, char_array, sizeof(char_array), 0, theirp->ai_addr, theirp->ai_addrlen)) == -1) {
        						perror("error: recv");
        						exit(1);
    						}
    						printf("A%d: Sent message %s to learner on %s\n", i, message.c_str(), learnerPorts[l]);
						}
						//exit(0);
					} else {
						string m = IGNORE;
						int n = m.length(); 
     			    	char char_array[n];
     			    	char_array[n-1] = '\0'; 
    					strcpy(char_array, m.c_str()); 
						if ((sentBytes = sendto(socket, char_array, sizeof(char_array), 0, (struct sockaddr *)&their_addr, addr_len)) == -1) {
	        				perror("error: sending");
        					exit(1);
    					}
    					printf("A%d: Sent response message %s to proposer\n", i, m.c_str());
					}
				}
			}
		}
	}
	while(1){

	}
}
