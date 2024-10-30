#include "csapp.h"
#include "cache.c"

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";

/* function and const variable declaration */
#define true 1
#define false 2

sem_t sem1;  // semaphore for listen file descriptor

void *thread(void *vargp);

void doit(int clientfd);
void getResponseToClient(int clientfd, char *request);
void sendRequestToServer(int clientfd, char* request, char* response);
void parseURI(char *uri, char *hostname, char *port);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);

int main(int argc, char **argv) {

    // check command-line argument is valid
    if(argc != 2) {
        fprintf(stderr, "usage: ./proxy <port>\n", argv[0]);
        exit(1);
    }

    int listenfd, *connfd;  // file descriptor for client
    char hostname[MAXLINE], port[MAXLINE];  // client hostname and port

    // socket variables for client
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;

    fd_set read_set;
    pthread_t tid;
    FD_ZERO(&read_set);
    FD_SET(listenfd, &read_set);

    // init semaphore
    Sem_init(&sem1, NULL, 1);
    Sem_init(&sem2, NULL, 1);

    initCache();  // init cache

    listenfd = Open_listenfd(argv[1]);  // listen from client
    while(true) {
        clientlen = sizeof(struct sockaddr_storage);
        P(&sem1);
        connfd = Malloc(sizeof(int));
        *connfd = Accept(listenfd, (SA*)&clientaddr, &clientlen);
        Getnameinfo((SA*)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
        printf("Accept connection from %s:%s\n", hostname, port);
        Pthread_create(&tid, NULL, thread, connfd);
    }
}

void *thread(void *vargp) {
    int connfd = *((int*)vargp);
    Pthread_detach(pthread_self());
    Free(vargp);
    V(&sem1);
    doit(connfd);
    Close(connfd);
}

void doit(int clientfd) {

    // request from client
    char request_from_client[MAXLINE] = { NULL };

    // response from server
    char response_form_server[MAXLINE];

    // connect to client and get request
    // change request version HTTP 1.1 to HTTP 1.0
    getResponseToClient(clientfd, request_from_client);
    if(request_from_client[0] == NULL)
        return;
    printf("Request after change version:\n");
    printf("%s\n\n", request_from_client);

    // connect to server and send request
    // get response from server and send to client
    sendRequestToServer(clientfd, request_from_client, response_form_server);
    printf("444444444444444444444444444444\n\n");
}

void getResponseToClient(int clientfd, char *request) {

    rio_t rio;  // object for read and write from client socket
    char buf[MAXLINE];  // temp buffer for client request

    // read request header from client
    Rio_readinitb(&rio, clientfd);
    Rio_readlineb(&rio, buf, MAXLINE);
    printf("Request header:\n");
    printf("%s\n", buf);
    
    // change HTTP 1.1 to HTTP 1.0
    char method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    sscanf(buf, "%s %s %s", method, uri, version);

    if(strcmp(method, "GET")) {
        clienterror(clientfd, method, "501", "Not implemented", "Tiny does not implement this method");
        return;
    }

    int idx = 0;
    idx += sprintf(request + idx, "%s %s %s\n", method, uri, "HTTP/1.0");
    while(strcmp(buf, "\r\n")) {
        Rio_readlineb(&rio, buf, MAXLINE);
        if(!strncasecmp(buf, "Connection", sizeof("Connection") - 1))
            idx += sprintf(request + idx, "Conection: close\r\n");
        else if(!strncasecmp(buf, "Upgrade-Insecure", sizeof("Upgrade-Insecure") - 1))
            continue;
        else if(!strncasecmp(buf, "Priority", sizeof("Priority") - 1))
            continue;
        else if(!strcmp(buf, "\r\n")) {
            idx += sprintf(request + idx, "Proxy-Connection: close\r\n");
            idx += sprintf(request + idx, buf);
            break;
        }
        else
            idx += sprintf(request + idx, buf);
    }

    return;
}

void sendRequestToServer(int clientfd, char *request, char *response) {

    rio_t rio;  // for read from server
    char buf[MAXBUF];  // temp buffer for server response
    int serverfd;  // file descriptor to server socket

    // for request to server
    char method[MAXLINE], uri[MAXLINE];
    sscanf(request, "%s %s %*s", method, uri);
    printf("method, uri: %s %s\n", method, uri);

    // check request is cached
    // if cached, response client from cache
    // if(checkCached(clientfd, method, uri)) {
    //     printf("%s %s is cached\n", method, uri);
    //     return;
    // }

    // server hostname and port number
    char hostname[MAXLINE], port[MAXLINE];

    // parse uri and get hostname, port
    parseURI(uri, hostname, port);
    printf("hostname: %s, port: %s\n", hostname, port);

    // connect and send request to server
    serverfd = Open_clientfd(hostname, port);
    Rio_writen(serverfd, request, strlen(request));

    // read response from server
    Rio_readinitb(&rio, serverfd);
    
    // resive http header from server
    char header[MAXLINE];
    int idx = 0;
    long int contentSize;
    do {
        Rio_readlineb(&rio, buf, MAXBUF);  // read line from server
        if(!strncmp(buf, "Content-length", 14))  // find http body len
            sscanf(buf, "%*s %ld", &contentSize);
        idx += sprintf(header + idx, buf);
        //printf("%s", buf);
        Rio_writen(clientfd, buf, strlen(buf));  // send line to client
    } while(strcmp(buf, "\r\n"));

    // resive http content from server
    printf("content size: %ld\n", contentSize);
    char* temp = (char*)Malloc(contentSize);
    Rio_readnb(&rio, temp, contentSize);

    // caching content and response to client
    //cached(clientfd, uri, header, temp, contentSize);
    Rio_writen(clientfd, temp, contentSize);
printf("222222222222222222222222222222\n\n");
    Free(temp);

    Close(serverfd);
printf("333333333333333333333333333333333333\n\n");
    return;
}

void parseURI(char *_uri, char *hostname, char *port) {

    char *uri;
    strcpy(uri, _uri);
printf("uri, _uri %s %s\n", uri, _uri);
    if(!strncmp(uri, "http", 4));
        uri = strstr(uri, "//") + 2;
    
    char *p = strstr(uri, "?");
    if(p != NULL)
        *p = '\0';
    p = strstr(uri, "/");
    *p = '\0';

    p = strstr(uri, ":");
    if(p == NULL) {
        strcpy(hostname, uri);
        strcpy(port, "80");
    }
    else {
        *p = '\0';
        strcpy(hostname, uri);
        strcpy(port, p + 1);
    }

    return;
}

void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg) {
    char buf[MAXLINE], body[MAXLINE];

    // Build the HTTP response body
    int idx = 0;
    idx += sprintf(body + idx, "<html><title>Tiny Error</title>");
    idx += sprintf(body + idx, "<body bgcolor=\"ffffff\">\r\n");
    idx += sprintf(body + idx, "%s: %s\r\n", longmsg, cause);
    idx += sprintf(body + idx, "<hr><em>The Tiny Web server</em>\r\n");

    // Print the HTTP response
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    Rio_writen(fd, buf, strlen(buf));
    Rio_writen(fd, body, strlen(body));
}