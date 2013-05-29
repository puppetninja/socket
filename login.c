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

#define PORT "21391"  // the port users will be connecting to
#define SUPERPORT "22391"//The port of super node
#define UDPORT "3391"//The UDP port the users will use.
#define MAXNAMELINE 1024
#define MAXDATASIZE 100//max number of bytes we can get at once
#define BACKLOG 10     // how many pending connections queue will hold

void sigchld_handler(int s)
{
    while(waitpid(-1, NULL, WNOHANG) > 0);
}

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
    int sockSupNode;
    struct addrinfo hints, hintsP2,*servinfo, *servinfoP2,*p;//*servinfoP2 will be used for Phase 2.
    struct sockaddr_storage their_addr; // connector's address information
    //struct sockaddr_in my_addr;//self addr info.
    socklen_t sin_size;
    //struct sigaction sa;
    char authen[5];  //The indicator of whether a user is accepted.
    int yes=1;
    char s[INET6_ADDRSTRLEN],sP2[INET6_ADDRSTRLEN];
    int rv,rvP2;//rvP2 will be used for Phase 2.
    char buf[MAXDATASIZE];
    char revMessage[MAXDATASIZE];
    int usercounter = 0;
    char user1Temp[MAXDATASIZE],user2Temp[MAXDATASIZE],user3Temp[MAXDATASIZE];
    char user1pass[MAXDATASIZE],user2pass[MAXDATASIZE],user3pass[MAXDATASIZE];
    char user1name[MAXDATASIZE],user2name[MAXDATASIZE],user3name[MAXDATASIZE];
    char user1pasWord[MAXDATASIZE],user2pasWord[MAXDATASIZE],user3pasWord[MAXDATASIZE];
    char user1ToSent[MAXDATASIZE],user2ToSent[MAXDATASIZE],user3ToSent[MAXDATASIZE];
    char superNode[MAXDATASIZE],superNodePort[MAXDATASIZE];//info to be sent which contains super node IP and Port Number.
    char userFname[MAXDATASIZE],userFpass[MAXDATASIZE];//username and password of failed login user.
    //The structure to get IP address of Super Node
    struct hostent *SupNode;
    struct in_addr **addr_list;
    //The structure to get to know self IP address and PORT number in Phase 2.
    struct sockaddr_in my_addr;
    socklen_t addrlen;

    //To Read UserMatchPass.txt
    FILE * pFile;
    pFile = fopen ("UserPassMatch.txt" , "r");
    if (pFile == NULL){
    	perror ("Error opening file");
    }
    else {
         if (fgets (user1pass , MAXDATASIZE , pFile) == NULL ){
        	 printf("error in int sa_len;reading files");
         }else{
        	 strcpy(user1Temp,user1pass);
        	 char * p;
        	 p = strtok(user1pass, " ");
        	 strcpy(user1name, p);
        	 p = strtok(NULL," ");
        	 strcpy(user1pasWord,p);
        	 strcpy(user1pass,user1Temp);
         }

         if(fgets (user2pass , MAXDATASIZE , pFile) == NULL ){
        	 printf("error in reading files");
         }else{
        	 strcpy(user2Temp,user2pass);
        	 char * p;
        	 p = strtok(user2pass, " ");
        	 strcpy(user2name, p);
        	 p = strtok(NULL," ");
        	 strcpy(user2pasWord,p);
        	 strcpy(user2pass,user2Temp);
         }

         if(fgets (user3pass , MAXDATASIZE , pFile) == NULL ){
        	 printf("error in reading files");
         }else{
        	 strcpy(user3Temp,user3pass);
        	 char * p;
        	 p = strtok(user3pass, " ");
        	 strcpy(user3name, p);
        	 p = strtok(NULL," ");
        	 strcpy(user3pasWord,p);
        	 strcpy(user3pass,user3Temp);
		 // strcat(user3pass,"\n"); //since for the last line, there is no '\n',it needs to be added.
         }

         fclose (pFile);
       }
     //The clear and socket creation block of code referenced from Beej's guide.
    //To clear the hints.
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP
    //
    char hostname[MAXNAMELINE];
    gethostname(hostname, sizeof(hostname));

    if ((rv = getaddrinfo(hostname, PORT, &hints, &servinfo)) != 0) {
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

        // get the pointer to the address itself,
        // different fields in IPv4 and IPv6:
        if (p->ai_family == AF_INET) { // IPv4
            struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
            addr = &(ipv4->sin_addr);
        } else { // IPv6
            struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
            addr = &(ipv6->sin6_addr);
        }
        // convert the IP to a string and char print it
        inet_ntop(p->ai_family, addr, s, sizeof s);

        printf("Phase1: LogIn Server has TCP port number %s and IP address  %s \n", PORT, s);

        break;
    }

    if (p == NULL)  {
        fprintf(stderr, "server: failed to bind\n");
        return 2;
    }

    //To get the address info of Super Node
    strcpy(superNodePort,"3391");//UDP port of super node is hard coded.
    int i;//iterator of printing IP.
    char hostnameSN[MAXNAMELINE];
    gethostname(hostnameSN, sizeof(hostnameSN));
    sP2[0] = '\0';// To clear the content of sP2.
    SupNode = gethostbyname(hostnameSN);
    addr_list = (struct in_addr **)SupNode->h_addr_list;
    for(i = 0; addr_list[i] != NULL; i++) {
        strcat(sP2,inet_ntoa(*addr_list[i]));
    }
    strcpy(superNode, sP2);
    strcat(superNode, " ");
    strcat(superNode, superNodePort);//superNode is the info of superNode to be sent.

    freeaddrinfo(servinfo); // all done with this structure

    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    while(usercounter != 3) {  // main accept() loop
        sin_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) {
            perror("accept");
            continue;
        }

        inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);
        //printf("server: got connection from %s\n", s);

        if ((numbytes = recv(new_fd, buf, MAXDATASIZE-1, 0)) == -1) {
            perror("recv");
            exit(1);
        }

        buf[numbytes] = '\0';
	// printf("Server data received: %s\n", buf);
        char * p;
        p = strtok(buf, "#");
        p = strtok(NULL, " ");
        strcpy(revMessage, p);
        p = strtok(NULL, " ");
        strcat(revMessage, " ");
        strcat(revMessage, p);
        //strcat(revMessage, "\n");
	//printf("revMesage is %s \n",revMessage);
	//printf("result of comp1 %d \n",strcmp(revMessage,user1pass));
	//printf("result of comp2 %d \n",strcmp(revMessage,user2pass));
	//printf("result of comp3 %d \n",strcmp(revMessage,user3pass));
        //The process of comparison.
        if(strcmp(revMessage,user1pass) == 0){
        	user1ToSent[0] = '\0';
        	strcat(user1ToSent,user1name);
        	strcat(user1ToSent," ");
        	strcat(user1ToSent,s);

        	strcpy(authen,"yes");//get authen "yes"
        	printf("Phase 1: Authentication request.\n");
        	printf("User: %s Password: %s User IP addr: %s \n",user1name,user1pasWord,s);
        	printf("Authorized: yes .\n");

        	if(send(new_fd,authen,4,0)==-1){
        	    perror("send");
        	}
        	if(send(new_fd,superNode,MAXDATASIZE-1,0)==-1){
        	    perror("send");
        	}
        	printf("Phase 1: SuperNode IP address: %s Port Number: %s sent to the <User#1> \n", sP2, superNodePort);

        }else if(strcmp(revMessage,user2pass) == 0){
        	user2ToSent[0] = '\0';
        	strcat(user2ToSent,user2name);
        	strcat(user2ToSent," ");
        	strcat(user2ToSent,s);

        	strcpy(authen,"yes");//get authen "yes"
        	printf("Phase 1: Authentication request.\n");
        	printf("User: %s Password: %s User IP addr: %s \n",user2name,user2pasWord,s);
        	printf("Authorized: %s .\n", authen);

        	if(send(new_fd,authen,4,0)==-1){
        	    perror("send");
        	}
        	if(send(new_fd,superNode,MAXDATASIZE-1,0)==-1){
        	   perror("send");
        	}
        	printf("Phase 1: SuperNode IP address: %s Port Number: %s sent to the <User#2> \n", sP2, superNodePort);

        }else if(strcmp(revMessage,user3pass) == 0){
        	user3ToSent[0] = '\0';
        	strcat(user3ToSent,user3name);
        	strcat(user3ToSent," ");
            strcat(user3ToSent,s);

            strcpy(authen,"yes");//get authen "yes"

            printf("Phase1: Authentication request.\n");
            printf("User: %s Password: %s User IP addr: %s \n",user3name,user3pasWord,s);
            printf("Authorized: %s .\n", authen);

            if(send(new_fd,authen,4,0)==-1){
                perror("send");
            }
            if(send(new_fd,superNode,MAXDATASIZE-1,0)==-1){
                perror("send");
            }
            printf("Phase 1: SuperNode IP address: %s Port Number: %s sent to the <User#3> \n", sP2, superNodePort);

        }else{
        	char * pFail;
        	pFail = strtok(revMessage, " ");
            strcpy(userFname, p);
        	pFail = strtok(NULL," ");
        	strcpy(userFpass,p);

        	strcpy(authen,"no");//get authen "no"
        	if(send(new_fd,authen,4,0)==-1){
        	   perror("send");
        	}
        	printf("Phase1: Authentication request.\n");
        	printf("User: %s Password: %s User IP addr: %s \n", userFname, userFpass, s);
        	printf("Authorized: %s .\n", authen);

        }

        close(new_fd);
        usercounter++;
    }


    close(sockfd);//close all the TCP connections to users.

    printf("End of Phase 1 for login server.\n");

    /*
     * Phase 2: Login server will connect to Super and Send the user/IP matching table
     * to Super Node.
     */
    //To get the information of Super Node.
    memset(&hintsP2, 0, sizeof hintsP2);
    hintsP2.ai_family = AF_UNSPEC;
    hintsP2.ai_socktype = SOCK_STREAM;
    //
    // char hostnameP2[MAXNAMELINE];
    //gethostname(hostnameP2, sizeof(hostnameP2));
    //printf("host name is %s \n",hostnameP2);

    if ((rvP2 = getaddrinfo("nunki.usc.edu", SUPERPORT, &hintsP2, &servinfoP2)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rvP2));
        return 1;
    }

    // loop through all the results to connect to the first we can
    for(p = servinfoP2; p != NULL; p = p->ai_next) {
        if ((sockSupNode = socket(p->ai_family, p->ai_socktype,p->ai_protocol)) == -1) {
            perror("Login as client: socket");
            continue;
        }

        if (connect(sockSupNode, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockSupNode);
            perror("Login as client: connect");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "Login as client: failed to connect to Super Node\n");
        return 3;
    }

    addrlen = sizeof(struct sockaddr);
    if(getsockname(sockSupNode,(struct sockaddr *)&my_addr,&addrlen) == -1){
       	perror("client:getsockname");
       	exit(1);
    }
    printf("Phase 2: Login server has TCP port %d and IP address %s \n", (int)ntohs(my_addr.sin_port), inet_ntoa(my_addr.sin_addr));
    freeaddrinfo(servinfoP2); // all done with this structure
    //The Process to sent the username/IP matching to Super Node.
    if(send(sockSupNode,user1ToSent,MAXDATASIZE-1,0) == -1){
        perror("Login as client:send");
    }

    if(send(sockSupNode,user2ToSent,MAXDATASIZE-1,0) == -1){
        perror("Login as client:send");
    }

    if(send(sockSupNode,user3ToSent,MAXDATASIZE-1,0) == -1){
        perror("Login as client:send");
    }

    printf("Phase 2:Login server sent %d username/IP address pairs to supernode\n",usercounter);
    close(sockSupNode);
    printf("End of Phase 2 for Login server.\n");
    //All finish for Login Server.
    return 0;
}

