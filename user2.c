/*
 * User2.c
 *
 *  Created on: Nov 19, 2011
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
#include <time.h> //the header file used for delay.

#define PORT "21391" // the port client will be connecting to on Login Server.
#define UDPORT "3591"//The UDP port the user will use.
#define MESSAGENUM 2
#define MAXNAMELINE 1024
#define MAXDATASIZE 100

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[])
{
    int sockfd, sockfdP3,sockfdSP;
    struct addrinfo hints, hintsP3, hintsSP, *servinfo, *servinfoP3, *servinfoSP, *p;
    struct sockaddr_in my_addr;
    struct sockaddr_storage their_addr;
    socklen_t addrlen, addrlenP3;
    int rv,rvP3,rvSP;
    int numbytes,numbytesSP;
    int messageCounter;
    char s[INET6_ADDRSTRLEN];
    char authen[5];
    char authenDeny[5];
    char userPass[80];
    char username[80];
    char password[80];
    char loginMessage[MAXDATASIZE];
    char superNode[MAXDATASIZE],superNoIP[MAXDATASIZE],superNoPort[MAXDATASIZE];
    char sendUser[MAXDATASIZE],revUser[MAXDATASIZE],content[MAXDATASIZE];
    char toUserMessage1[MAXDATASIZE],toUserMessage2[MAXDATASIZE];
    char toUserMessage1BK[MAXDATASIZE],toUserMessage2BK[MAXDATASIZE];
    char fromUserMessage[MAXDATASIZE];
    char termReq[MAXDATASIZE];

    // char hostname[MAXNAMELINE];
    //gethostname(hostname, sizeof(hostname));
    //The clear and socket creation block of code referenced from Beej's guide.
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo("nunki.usc.edu", PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and connect to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,p->ai_protocol)) == -1) {
            perror("User#2: socket");
            continue;
        }

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("User#2: connect");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "User#2: failed to connect\n");
        return 2;
    }

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),s, sizeof s);
    //printf("User#2: connecting to %s\n", s);
    //To get the TCP port number and Local IP address.
    addrlen = sizeof(struct sockaddr);
    if(getsockname(sockfd,(struct sockaddr *)&my_addr,&addrlen) == -1){
    	perror("User#2:getsockname");
    	exit(1);
    }

    printf("Phase 1: <User#2> has TCP port %d and IP address %s \n", (int)ntohs(my_addr.sin_port), inet_ntoa(my_addr.sin_addr));

    freeaddrinfo(servinfo); // all done with this structure
    //To read the userPass.txt
        FILE * pFile;
        pFile = fopen ("UserPass2.txt" , "r");

        if (pFile == NULL){
            perror ("Error opening file");
        }
        else{
            if (fgets (userPass , 80 , pFile) == NULL ){
                printf("error in reading files");
            }else{
                char * p;
                p = strtok(userPass, " ");
                strcpy(username, p);
                p = strtok(NULL, " ");
                strcpy(password, p);
            }
        }
        loginMessage[0] = '\0';
        strcat(loginMessage,"Login#");
        strcat(loginMessage,username);
        strcat(loginMessage," ");
        strcat(loginMessage,password);

    //To send the login message to the login server.
    printf("Phase 1:<User#2>Login request.User: %s  password: %s .\n", username, password);
    if(send(sockfd,loginMessage,MAXDATASIZE-1,0) == -1){
    	perror("User#2:send");
    	exit(1);
    }

    //To receive the Super Node info message.
    if(recv(sockfd,authen,4,0) == -1){
    	perror("User#2: receive authen");
    	exit(1);
    }
    printf("Phase 1:Login request reply: %s \n",authen);

    strcpy(authenDeny,"yes");
    if(strcmp(authen,authenDeny) == 0 ){
       if(recv(sockfd,superNode,MAXDATASIZE-1,0) == -1){
    	    perror("User#2:receive");
    	    exit(1);
        }
       //Process to segment the Super Node IP address and Port Number.
       char * pSeg;
       pSeg = strtok(superNode, " ");
       strcpy(superNoIP, pSeg);
       pSeg = strtok(NULL, " ");
       strcpy(superNoPort, pSeg);
    //Print info
    printf("Phase 1:Supernode has IP address %s , and Port Number %s .\n", superNoIP, superNoPort);
    }
    //The clear and socket creation block of code referenced from Beej's guide.
    //To create an UDP socket using static port number.
    memset(&hintsP3, 0, sizeof hintsP3);
    hintsP3.ai_family = AF_UNSPEC; // set to AF_INET to force IPv4
    hintsP3.ai_socktype = SOCK_DGRAM;
    hintsP3.ai_flags = AI_PASSIVE; // use my IP

    if ((rvP3 = getaddrinfo(NULL, UDPORT, &hintsP3, &servinfoP3)) != 0) {
         fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rvP3));
         return 1;
    }
   // loop through all the results and bind to the first we can

    for(p = servinfoP3; p != NULL; p = p->ai_next) {
        if ((sockfdP3 = socket(p->ai_family, p->ai_socktype,p->ai_protocol)) == -1) {
            perror("User#2: socket");
            continue;
        }

        if (bind(sockfdP3, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfdP3);
            perror("User#2: bind");
            continue;
         }

        break;
    }

    if (p == NULL) {
         fprintf(stderr, "User#2: failed to bind socket\n");
         return 2;
    }
    freeaddrinfo(servinfoP3);//free the infomation of User1 on static UDP port.
    printf("Phase 1:<User#2> is listening on port number %s .\n",UDPORT);

    close(sockfd);//close TCP socket connection to the Login Server.

    printf("End of Phase 1 for <User#2>.\n");
    //To get the IP address of User#2 through UDP socket.
    if(getsockname(sockfdP3,(struct sockaddr *)&my_addr,&addrlen) == -1){
       	perror("User#2:getsockname");
       	exit(1);
       }
    printf("Phase 3: <User#2> has UDP port %s and IP address %s \n", UDPORT, inet_ntoa(my_addr.sin_addr));
    //To set a delay which makes the user start to send the text
    sleep(10);
    //To get create a socket before sending the message to the Super Node.
    memset(&hintsSP, 0, sizeof hintsSP);
    hintsSP.ai_family = AF_UNSPEC;
    hintsSP.ai_socktype = SOCK_DGRAM;

    if ((rvSP = getaddrinfo(superNoIP, superNoPort, &hintsSP, &servinfoSP)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rvSP));
        return 3;
    }

    for(p = servinfoSP; p != NULL; p = p->ai_next) {
        if ((sockfdSP = socket(p->ai_family, p->ai_socktype,p->ai_protocol)) == -1) {
             perror("User#2: socket");
             continue;
        }
        break;
    }

    if (p == NULL) {
        fprintf(stderr, "User#2: failed to bind socket\n");
        return 4;
    }
    //To read the input test file and sent the message to the Super Node.
    FILE * pFileP3;
    pFileP3 = fopen ("UserText2.txt" , "r");
    if (pFile == NULL){
       	perror ("Error opening file");
    }else{
         if (fgets (toUserMessage1 , MAXDATASIZE , pFileP3) == NULL ){
           	 printf("error in reading files");
         }else{
        	 strcpy(toUserMessage1BK,toUserMessage1);
        	 //printf("the message to be sent %s ",toUserMessage1);
        	 char * pSeg;
        	 pSeg = strtok(toUserMessage1, "-");
        	 strcpy(revUser,  pSeg);
        	 pSeg = strtok(NULL,":");
        	 strcpy(sendUser, pSeg);
        	 if((numbytes = sendto(sockfdSP,toUserMessage1BK,strlen(toUserMessage1BK),0,p->ai_addr, p->ai_addrlen)) == -1){
        		 perror("User#2:send to Super Node");
        		 exit(1);
        	 }
        	 printf("Phase 3:<User#2> is sending the message: %s ",toUserMessage1BK);
         }

         if (fgets (toUserMessage2 , MAXDATASIZE , pFileP3) == NULL ){
             printf("error in reading files");
         }else{
        	 strcpy(toUserMessage2BK,toUserMessage2);
        	 char * pSeg;
        	 pSeg = strtok(toUserMessage2, "-");
        	 strcpy(revUser,  pSeg);
        	 pSeg = strtok(NULL,":");
        	 strcpy(sendUser, pSeg);
        	 strcat(toUserMessage2BK,"\n");//To add a sequence to the end of second string.
        	 if((numbytes = sendto(sockfdSP,toUserMessage2BK,strlen(toUserMessage2BK),0,p->ai_addr, p->ai_addrlen)) == -1){
        	     perror("User#2:send to Super Node");
        	     exit(1);
        	     printf("message send failure.\n");
        	 }
        	 printf("Phase 3:<User#2> is sending the message: %s \n",toUserMessage2BK);
         }
    }
    freeaddrinfo(servinfoSP);
    //Wait for messages from SuperNode.
    //printf("before receiving stage of User2.\n");
    sleep(4);//to make a delay gap between the send and receive section.
    //printf("User#2:waiting for the Super Node to send packet....");
    addrlenP3 = sizeof their_addr;
    while(messageCounter != MESSAGENUM){
    	if ((numbytesSP = recvfrom(sockfdP3, fromUserMessage, MAXDATASIZE-1 , 0,(struct sockaddr *)&their_addr, &addrlenP3)) == -1) {
    	        perror("recvfrom");
    	        exit(1);
    	}
    	char * pSeg;
    	pSeg = strtok(fromUserMessage, "-");
    	strcpy(revUser,  pSeg);
    	pSeg = strtok(NULL,"\n");
    	strcpy(content, pSeg);
    	//pSeg = strtok(NULL," ");
    	//strcpy(content, pSeg);
    	printf("Phase 3:<User#2> received the message: %s \n",content);
    	messageCounter++;
    }
    //Send Terminate request to the Super Node.
    strcpy(termReq,"terminate\n");
    if((numbytes = sendto(sockfdSP,termReq,strlen(termReq),0,p->ai_addr, p->ai_addrlen)) == -1){
        perror("User#2:send terminate request");
        exit(1);
    }
    //
    close(sockfdP3);
    close(sockfdSP);
    printf("End of Phase 3 for <User#2>.\n");
    return 0;
}

