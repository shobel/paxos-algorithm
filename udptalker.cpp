/*
** talker.c -- a datagram "client" demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sstream>

using namespace std;
//const char* ports[] = {"4064", "4164", "4264"};
const char* ports[] = {"9000", "9001"};

int main(int argc, char *argv[])
{
    for (int z = 0; z < 2; z++){
        if (!fork()){

    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    int numbytes;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;

    char hostname[1024];
    hostname[1023] = '\0';
    gethostname(hostname, strlen(hostname));

    if ((rv = getaddrinfo(hostname, ports[z], &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and make a socket
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("talker: socket");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "talker: failed to create socket\n");
        return 2;
    }
    printf("Got socket %d on port %s\n", sockfd, ports[z]);

    stringstream ss;
    ss << z;
    string message;
    ss >> message;
    if ((numbytes = sendto(sockfd, message.c_str(), strlen(message.c_str()), 0,
             p->ai_addr, p->ai_addrlen)) == -1) {
        perror("talker: sendto");
        exit(1);
    }

    freeaddrinfo(servinfo);

    printf("talker: sent %d bytes\n", numbytes);
    close(sockfd);

    return 0;
    
    }
    }
    while(1){}
}