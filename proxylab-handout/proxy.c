#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

#define TINY_HOST_NAME "localhost"
#define TINY_LISTEN_PORT "23456"

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

void handleRequest(int connfd);
void clientError(int fd, char* cause, char* errnum, char* shortmsg, char* longmsg);
void readRequestHeader(rio_t* rp, char* clientRequestP);
int checkGetMethod(char* uri, char* fileName, char* cgiargs);
void readResponseHeader(char* tinyResponseP, rio_t* rio);


void handleRequest(int fd){
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char clientRequest[MAXLINE], *clientRequestP = clientRequest;
    rio_t rio;

    Rio_readinitb(&rio, fd);

    /** step1: read request from client */
    if(Rio_readlineb(&rio, buf, MAXLINE) == 0){
        //the request has the empty space
        printf("empty request\n");
        return;
    }

    printf("-------- client request first line start -----------\n");
    printf("%s", buf);
    strcpy(clientRequest, buf);
    clientRequestP += strlen(buf);
    printf("-------- client request first line end-----------\n");

    sscanf(buf, "%s %s %s", method, uri, version);

    /** step2: determine if it is a valid request */
    if(strcasecmp(method, "GET") != 0){
        clientError(fd, method, "501", "Not Implemented", "Tiny Does not implement this method");
        return;
    }

    readRequestHeader(&rio, clientRequestP);

    /** step3: establish own connection with tiny server
     *         and forward the request to the client
     * */
    printf("what we got when reaching here ----- \n");
    printf("%s", clientRequest);

    char* hostName = TINY_HOST_NAME;
    char* port = TINY_LISTEN_PORT;
    int clientfd = Open_clientfd(hostName, port);


    rio_t rioTiny;
    Rio_readinitb(&rioTiny, clientfd);
    Rio_writen(rioTiny.rio_fd, clientRequest, strlen(clientRequest));

    /** step4: read the response from tiny and send it to the client */
    char tinyResponse[MAXLINE];
    char* tinyResponseP = tinyResponse;

    readResponseHeader(tinyResponseP, &rioTiny);
    /* send it to the client */
    Rio_writen(rio.rio_fd, tinyResponse, strlen(tinyResponse));
}

void readResponseHeader(char* tinyResponseP, rio_t* rio){
    char buf[MAXLINE];
    while( (Rio_readlineb(rio, buf, MAXLINE)) != 0 ){
        strcpy(tinyResponseP, buf);
        tinyResponseP += strlen(buf);

        /* have met the header */
        if(strcmp(buf, "\r\n") == 0)
            break;
    }

    while( (rio_readlineb(rio, buf, MAXLINE)) != 0){
        strcpy(tinyResponseP, buf);
        tinyResponseP += strlen(buf);
    }
}

void readRequestHeader(rio_t* rp, char* clientRequestP){
    char buf[MAXLINE];
    Rio_readlineb(rp, buf, MAXLINE);

    strcpy(clientRequestP, buf);
    clientRequestP += strlen(buf);

    printf("%s", buf);
    while(strcmp("\r\n", buf) != 0){
        Rio_readlineb(rp, buf, MAXLINE);

        strcpy(clientRequestP, buf);
        clientRequestP += strlen(buf);

        printf("%s", buf);
    }

    return;
}

void clientError(int fd, char* cause, char* errnum, char* shortmsg, char* longmsg){
    char buf[MAXLINE];

    /* Print the HTTP response headers */
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n\r\n");
    Rio_writen(fd, buf, strlen(buf));

    /* Print the HTTP response body */
    sprintf(buf, "<html><title>Tiny Error</title>");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "<body bgcolor=""ffffff"">\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "%s: %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "<p>%s: %s\r\n", longmsg, cause);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "<hr><em>The Tiny Web server</em>\r\n");
    Rio_writen(fd, buf, strlen(buf));
}

int main(int argc, char** argv){
    if(argc != 2){
        unix_error("proxy usage: ./proxy <port number>");
    }

    int listenfd = Open_listenfd(argv[1]);
    struct sockaddr_storage clientAddr;
    char hostName[MAXLINE], port[MAXLINE];

    while(1){
        socklen_t addrLen = sizeof(struct sockaddr_storage);
        int connfd = Accept(listenfd, (SA*)&clientAddr, &addrLen);

        Getnameinfo((SA*)&clientAddr, addrLen, hostName, MAXLINE, port, MAXLINE, 0);
        printf("Accepting Connection from (%s, %s) \n", hostName, port);

        handleRequest(connfd);
        Close(connfd);
    }


    printf("%s", user_agent_hdr);
    return 0;
}
