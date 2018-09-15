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
#define MAX_PROPOSAL_NUM 9
#define MAXBUFLEN 5
#define NUM_PROPOSERS 2
#define NUM_ACCEPTORS 3
#define PHASE_PREPARE "p"
#define PHASE_ACCEPT "a"
#define IGNORE "ignor"
#define ACCEPT "accep"

const char* proposerPorts[] = {"9000", "9001"};
const char* acceptorPorts[] = {"4064", "4164", "4264"};
//vector<string> acceptorPorts(ap, end(ap));

struct Proposal {
	string phase;
    int number;   
    int value;  
}; 

int myrandom(int i) { return rand()%i;}

int toInt(char* input, int &output){
	stringstream ss;
	ss << input;
	ss >> output;
	return output;
}

int getRandomValue(){
	return rand()%(MAX_PROPOSAL_VAL - 1 + 1) + 1;
}

int getRandomStartingNumber(){
	return rand()%(3 - 1 + 1) + 1;
}

int getRandomIncrement(){
	return rand()%(2 - 1 + 1) + 1;
}

Proposal createProposal(int pnum, int value, string phase){
	Proposal proposal;
	proposal.phase = phase;
	proposal.number = pnum;
	proposal.value = value;
	return proposal;
}

Proposal messageToProposal(char buf[], int num, int val){
	Proposal p;

	if (strcmp(buf, "none") == 0){
		p.phase = PHASE_ACCEPT;
		p.number = num;
		p.value = val;//getRandomValue();
		return p;
	}

	stringstream ss;
  	char* token;
  	token = strtok (buf,":");
  	int counter = 0;
  	while (token != NULL) {
  		//printf("token is %s\n", token);
  		istringstream ss(token);
	 	if (counter == 0){
  			ss >> p.phase;
  		} else if (counter == 1){
  			ss >> p.number;
  		} else if (counter == 2){
  			ss >> p.value;
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

int bindSocket(const char* proposerPort, addrinfo *&p){
	int sockfd, rv;
	struct addrinfo hints, *servinfo;
	memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; 
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE; 

    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 500000;

	if ((rv = getaddrinfo(NULL, proposerPort, &hints, &servinfo)) != 0) {
       	exit(1);
    }
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            continue;
        }
        
        //if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        //    perror("bind socket error");
        //}

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
    return sockfd;
}

int getSocket(const char* port, addrinfo *&p){
	int sockfd, rv;
    struct addrinfo hints, *servinfo;
    struct hostent *h;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;

    struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 500000;

    char hostname[1024];
    hostname[1023] = '\0';
    gethostname(hostname, 1024);
    h = gethostbyname(hostname);
    char* ip = inet_ntoa(*((struct in_addr *)h->h_addr));

    if ((rv = getaddrinfo(ip, port, &hints, &servinfo)) != 0) {
        exit(1);
   	}
    for(p = servinfo; p != NULL; p = p->ai_next) {
    	if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
        	continue;
        }
        //if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        //    perror("get socket error");
        //}

        break;
    }
    if (p == NULL) {
        exit(1);		
    }
    //printf("Returning socket %d on port %s\n", sockfd, port);
    return sockfd;
}

int main(int argc, char *argv[]){

	int num_proposers = NUM_PROPOSERS;
	int num_acceptors = NUM_ACCEPTORS;
	stringstream argstream;
	if (argc == 4){
		argstream << argv[1];
		argstream >> num_proposers;
		argstream.clear();
		argstream << argv[2];
		argstream >> num_acceptors;
	}

	//fork off the proposers
	for (int i = 0; i < num_proposers; i++){
		if (!fork()){

			srand(time(0) + i);
			struct addrinfo *myp;
			socklen_t addr_len;
			struct sockaddr_storage their_addr;
   			char buf[MAXBUFLEN];
			int receiveBytes, receiveSocket;
			const char* port = proposerPorts[i];

			//listen for responses first so we can get them as soon as we send proposals
			//receiveSocket = bindSocket(port, myp);			
			//printf("Listening to socket %d on port %s\n", receiveSocket, proposerPorts[i]);

			//randomly pick majority of acceptors
			//int numChosen = ceil((float) num_acceptors * .5);
   			//random_shuffle(acceptorPorts.begin(), acceptorPorts.end(), myrandom);

   			//start in prepare phase, this will get updated constantly inside while loop 
   			string phase = PHASE_PREPARE;
   			int lastProposalNum = 0;
   			int proposalVal = 0;
   			vector<Proposal> receivedProposals;

   			//send messages to chosen acceptors and receive responses, then send more messages and receive responses
   			while (1) {

   				Proposal proposal;
				if (phase == PHASE_PREPARE){
					//if in prepare phase, create a proposal
					//printf("P%d: doing prepare phase stuff\n", i);
					proposalVal = getRandomValue();
					int newProposalNumber = lastProposalNum + getRandomIncrement();
					if (newProposalNumber > 9){
						newProposalNumber = 9;
					} else if (lastProposalNum == 0) {
						newProposalNumber = getRandomStartingNumber();
					}
					proposal = createProposal(newProposalNumber, proposalVal, PHASE_PREPARE);
					lastProposalNum = newProposalNumber;
				} else {
					//if accept phase, find highest proposal received to offer
					//printf("P%d: doing accept phase stuff\n", i);
					int highestProposalNum = 0;
					int highestProposalValue = 0;
					for (int p = 0; p < receivedProposals.size(); p++){
						Proposal vectorp = receivedProposals[p];
						//printf("P%d: checking received proposal %s:%d:%d\n", i, receivedProposals[p].phase.c_str(), receivedProposals[p].number, receivedProposals[p].value);
						if (vectorp.number >= highestProposalNum){
							highestProposalNum = vectorp.number;
							if (vectorp.value > highestProposalValue){
								highestProposalValue = vectorp.value;
							}
						}
					}
					if (highestProposalValue == 0){
						highestProposalValue = rand()%(MAX_PROPOSAL_VAL - 0 + 1) + 0;
					}
					proposal = createProposal(lastProposalNum, highestProposalNum, PHASE_ACCEPT);
				}

				int counter = 0;
				int numAcceptsReceived = 0;
				receivedProposals.clear();
				//printf("P%d: number of acceptors to send to %d\n", i, numChosen);
				for (int a = 0; a < num_acceptors; a++){
					//printf("P%d: iteration %d\n", i, a);
					struct addrinfo *theirp;
					int sentBytes, sendSocket;
					sendSocket = getSocket(acceptorPorts[a], theirp);
					//printf("Got socket %d on port %s\n", sendSocket, acceptorPorts[a].c_str());

					//send prepare message to acceptor
					//printf("sending a %s message\n", proposal.phase.c_str());
    				string message = proposalToMessage(proposal);
    			 	int n = message.length(); 
     			    char char_array[n];
     			    char_array[n-1] = '\0'; 
    				strcpy(char_array, message.c_str()); 
    				if ((sentBytes = sendto(sendSocket, char_array, sizeof(char_array), 0, theirp->ai_addr, theirp->ai_addrlen)) == -1) {
        				perror("error: send");
        				exit(1);
    				}
  		  			printf("P%d: Sent message %s to acceptor on %s\n", i, char_array, acceptorPorts[a]);
    				addr_len = sizeof(their_addr);
  
   					if ((receiveBytes = recvfrom(sendSocket, buf, MAXBUFLEN, 0, (struct sockaddr *)&their_addr, &addr_len)) == -1) {	        				//printf("P%d: timeout\n", i);
	        			perror("error: recv");
	        			continue;
	    			}
	    			if (receiveBytes == 0) {
	    				perror("error: recv");
	    				continue;
	    			}
	    			if (strcmp(buf, IGNORE) == 0) {
	    				printf("P%d: no response, timed out\n", i);
	    				usleep(5000000);
	    				continue;
	    			} else if (strcmp(buf, ACCEPT) == 0){
	    				printf("P%d: accepted!\n", i);
	    				numAcceptsReceived++;
	    				if (proposal.phase == PHASE_ACCEPT && numAcceptsReceived == NUM_ACCEPTORS){
	    					printf("P%d: done!\n", i);
	    					exit(1);
	    				}
	    				continue;
	    			}
	    			printf("P%d: Received message %s\n", i, buf);
	    			Proposal receivedProposal = messageToProposal(buf, lastProposalNum, proposalVal);
	    			//printf("P%d: turned it into a proposal %s:%d:%d\n", i, receivedProposal.phase.c_str(), receivedProposal.number, receivedProposal.value);
	    			receivedProposals.push_back(receivedProposal);
	    			counter++;
    			}
    			//update phase for next round
    			if (counter == num_acceptors){
    				//printf("P%d: switching phase to accept\n", i);
    				phase = PHASE_ACCEPT;
    			} else {
    				phase = PHASE_PREPARE;
    			}
    		}
		}
	}
	while(1){

	}
}
