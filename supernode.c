/*
 * Supernode.c
 *
 *
 *
 *  Created on: Nov 17, 2011
 *      Author: chao
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#define SNPORT "22391" // the port users will be connecting to
#define UDPORT "3391" //The UDP port the users will use.
#define USER1PORT "3491" //The UDP Port of User1
#define USER2PORT "3591" //The UDP Port of User2
#define USER3PORT "3691" //The UDP Port of User3
#define MAXNAMELINE 1024
#define MAXDATASIZE 100//max number of bytes we can get at once
#define TERMREQNUM 3 //the number of terminate request.
#define BACKLOG 10     // how many pending connections queue will hold

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(void)
{
    int sockfd, new_fd, numbytes;  // listen on sock_fd, new connection on new_fd
    int sockfdP3, sockfdSP;//the socket descriptor that will be used in phase 3.
    struct addrinfo hints, hintsP3, hintsSP, *servinfo, *servinfoP3, *servinfoSP, *p;
    struct sockaddr_storage their_addr,user_addr; // connector's address information
    socklen_t sin_size;
    socklen_t addrlen,addr_user;
    int yes=1;
    int usercounter = 0;
    int termReqCounter = 0;
    char s[INET6_ADDRSTRLEN];
    int rv, rvP3, rvSP;
    char user1info[MAXDATASIZE],user2info[MAXDATASIZE],user3info[MAXDATASIZE];
    char * pInfo,*pMessage;
    char user1name[MAXDATASIZE],user2name[MAXDATASIZE], user3name[MAXDATASIZE];
    char user1IP[MAXDATASIZE],user2IP[MAXDATASIZE],user3IP[MAXDATASIZE];
    char revUser[MAXDATASIZE];//, content[MAXDATASIZE];//sendUser[MAXDATASIZE]
    char comMessage[MAXDATASIZE];
    char comMessageBK[MAXDATASIZE];
    char termReq[MAXDATASIZE];
    int numbytesP3, numbytesSP;
    struct sockaddr_in my_addr;

    char hostname[MAXNAMELINE];
    gethostname(hostname, sizeof(hostname));

    //The clear and socket creation block of code referenced from Beej's guide.
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(hostname, SNPORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
    	void *addr;
        if ((sockfd = socket(p->ai_family, p->ai_socktype,p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }

        if (p->ai_family == AF_INET) { // IPv4
            struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
            addr = &(ipv4->sin_addr);
        } else { // IPv6
            struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
            addr = &(ipv6->sin6_addr);
        }
        // convert the IP to a string and char print it
        inet_ntop(p->ai_family, addr, s, sizeof s);
        printf("Phase 2: SuperNode has TCP port number %s and IP address  %s \n", SNPORT, s);

        break;
    }

    if (p == NULL)  {
        fprintf(stderr, "server: failed to bind\n");
        return 2;
    }

    freeaddrinfo(servinfo); // all done with this structure

    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }
    //The Process to accept the information sent by Login server
    sin_size = sizeof their_addr;
    new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);

    if (new_fd == -1) {
        perror("accept");
    }

    inet_ntop(their_addr.ss_family,get_in_addr((struct sockaddr *)&their_addr),s, sizeof s);
    //printf("server: got connection from %s\n", s);
    //receive section in Phase 2 of Super Node.
    //For first message.
    if ((numbytes = recv(new_fd, user1info, MAXDATASIZE-1, 0)) == -1) {
        perror("recv");
        exit(1);
    }
    pInfo = strtok(user1info, " ");
    strcpy(user1name, pInfo);
    pInfo = strtok(NULL," ");
    strcpy(user1IP,pInfo);
    usercounter++ ;
    //For second message
    if ((numbytes = recv(new_fd, user2info, MAXDATASIZE-1, 0)) == -1) {
        perror("recv");
        exit(1);
    }
    pInfo = strtok(user2info, " ");
    strcpy(user2name, pInfo);
    pInfo = strtok(NULL," ");
    strcpy(user2IP,pInfo);
    usercounter++ ;
    //For third User.
    if ((numbytes = recv(new_fd, user3info, MAXDATASIZE-1, 0)) == -1) {
        perror("recv");
        exit(1);
    }
    pInfo = strtok(user3info, " ");
    strcpy(user3name, pInfo);
    pInfo = strtok(NULL," ");
    strcpy(user3IP,pInfo);
    usercounter++ ;

    close(new_fd);
    close(sockfd);//close all the TCP connections.
    printf("Phase 2:Super Node received received %d username/IP address pairs.\n",usercounter);
    printf("End of Phase 2 for Super Node.\n");
    //The clear and socket creation block of code referenced from Beej's guide.
    //The Section of Phase 3
    memset(&hintsP3, 0, sizeof hintsP3);
    hintsP3.ai_family = AF_UNSPEC; // set to AF_INET to force IPv4
    hintsP3.ai_socktype = SOCK_DGRAM;
    hintsP3.ai_flags = AI_PASSIVE; // use my IP

    if ((rvP3 = getaddrinfo(NULL, UDPORT, &hintsP3, &servinfoP3)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rvP3));
        return 3;
    }

    for(p = servinfoP3; p != NULL; p = p->ai_next) {
        if ((sockfdP3 = socket(p->ai_family, p->ai_socktype,p->ai_protocol)) == -1) {
             perror("Super Node: socket");
             continue;
        }

        if (bind(sockfdP3, p->ai_addr, p->ai_addrlen) == -1) {
             close(sockfdP3);
             perror("Super Node: bind");
             continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "Super Node: failed to bind socket\n");
        return 4;
    }
    //To get the self information about address.
    addrlen = sizeof(struct sockaddr);
    if(getsockname(sockfdP3,(struct sockaddr *)&my_addr,&addrlen) == -1){
        perror("Super Node:getsockname");
        exit(1);
    }
    printf("Phase 3: Super Node has UDP port %d and IP address %s \n", (int)ntohs(my_addr.sin_port), inet_ntoa(my_addr.sin_addr));
    freeaddrinfo(servinfoP3);//free the infomation of User1 on static UDP port.
    //To message deal stage
    strcpy(termReq,"terminate\n");
    addr_user = sizeof user_addr;
    while(termReqCounter != TERMREQNUM){
    	memset(comMessage, 0,  sizeof(char) * sizeof(comMessage));
    	memset(comMessageBK, 0,  sizeof(char) * sizeof(comMessageBK));
    	if((numbytesP3 = recvfrom(sockfdP3, comMessage, MAXDATASIZE-1, 0,(struct sockaddr *)&user_addr, &addr_user)) == -1){
    		perror("Super Node:recvfrom");
    		exit(1);
    	}
        printf("Phase 3:SuperNode received the message: %s ",comMessage);
        strcpy(comMessageBK,comMessage);//compy the content of message before it is segmented.
        if((strcmp(comMessage,termReq)) == 0){
        	termReqCounter++;
        }else{
        	//clear the structure
        	memset(&hintsSP, 0, sizeof hintsSP);
        	hintsSP.ai_family = AF_UNSPEC;
        	hintsSP.ai_socktype = SOCK_DGRAM;

        	pMessage = strtok(comMessage,"-");
        	strcpy(revUser,comMessage);

        	if((strcmp(revUser,user1name)) == 0){
        		if ((rvSP = getaddrinfo(user1IP, USER1PORT, &hintsSP, &servinfoSP)) != 0) {
        		    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        		    return 5;
        		}

        		for(p = servinfoSP; p != NULL; p = p->ai_next){
        		    if ((sockfdSP = socket(p->ai_family, p->ai_socktype,p->ai_protocol)) == -1) {
        		        perror("Super Node: socket");
        		        continue;
        		    }

        		    break;
        		}

        		if (p == NULL) {
        		    fprintf(stderr, "Super Node: failed to bind socket\n");
        		    return 6;
        		}

        		if ((numbytesSP = sendto(sockfdSP, comMessageBK, strlen(comMessageBK), 0, p->ai_addr, p->ai_addrlen)) == -1) {
        		     perror("Super Node: send to <User#1>");
        		     exit(1);
        		}
                printf("Phase 3: Super Node sent the message: %s ",comMessageBK);
                close(sockfdSP);
        	}else if((strcmp(revUser,user2name)) == 0){
        		if ((rvSP = getaddrinfo(user2IP, USER2PORT, &hintsSP, &servinfoSP)) != 0) {
        		    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        		    return 7;
        		}

         		for(p = servinfoSP; p != NULL; p = p->ai_next){
        		     if ((sockfdSP = socket(p->ai_family, p->ai_socktype,p->ai_protocol)) == -1) {
        		     perror("Super Node: socket");
        		     continue;
        	         }
        		     break;
         		}

         		if (p == NULL) {
        		    fprintf(stderr, "Super Node: failed to bind socket\n");
        		    return 8;
        		}

        		if ((numbytesSP = sendto(sockfdSP, comMessageBK, strlen(comMessageBK), 0, p->ai_addr, p->ai_addrlen)) == -1) {
        		    perror("Super Node: send to User<#2>");
        		    exit(1);
        		}
        		printf("Phase 3: Super Node sent the message: %s ",comMessageBK);
        		close(sockfdSP);
        	}else if((strcmp(revUser,user3name)) == 0){
        		if ((rvSP = getaddrinfo(user3IP, USER3PORT, &hintsSP, &servinfoSP)) != 0) {
        		    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        		    return 9;
        		}

        		for(p = servinfoSP; p != NULL; p = p->ai_next){
        		    if ((sockfdSP = socket(p->ai_family, p->ai_socktype,p->ai_protocol)) == -1) {
        		        perror("Super Node: socket");
        		        continue;
        		    }
        		    break;
        		}

        		if (p == NULL) {
        		    fprintf(stderr, "Super Node: failed to bind socket\n");
        		    return 10;
        		}

        		if ((numbytesSP = sendto(sockfdSP, comMessageBK, strlen(comMessageBK), 0, p->ai_addr, p->ai_addrlen)) == -1) {
        		    perror("Super Node: send to User<#2>");
        		    exit(1);
        		}
        		printf("Phase 3: Super Node sent the message: %s ",comMessageBK);
        		close(sockfdSP);
        	}else{
        		printf("Destination Error.");
        	}
        }
    }
    //
    close(sockfdP3);
    printf("End of Phase 3 for SuperNode");
    return 0;
}

