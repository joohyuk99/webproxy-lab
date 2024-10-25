#include "csapp.h"

void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);

int main(int argc, char **argv) {
    int listenfd, connfd;
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;

    // check command-line argument
    if(argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }

    listenfd = Open_listenfd(argv[1]);
    while(1) {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA*)&clientaddr, &clientlen);
        Getnameinfo((SA*)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
        printf("Accept connection from (%s, %s)\n", hostname, port);
        doit(connfd);
        Close(connfd);
    }
}

void doit(int fd) {
    int is_static;
    struct stat sbuf;
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char filename[MAXLINE], cgiargs[MAXLINE];
    rio_t rio;

    // Read request line and headers
    Rio_readinitb(&rio, fd);
    Rio_readlineb(&rio, buf, MAXLINE);
    printf("Request headers:\n");
    printf("%s", buf);
    sscanf(buf, "%s %s %s", method, uri, version);
    if(strcasecmp(method, "GET")) {
        clienterror(fd, method, "501", "Not implemented", "Tiny does not implement this method");
        return;
    }
    read_requesthdrs(&rio);

    // Parse URI from GET request
    is_static = parse_uri(uri, filename, cgiargs);
    printf("filename: %s\n", filename);
    if(stat(filename, &sbuf) < 0) {
        clienterror(fd, filename, "404", "Not found", "Tiny couldn't read the file");
        return;
    }

    if(is_static) {  // serve static content
        if(!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) {
            clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't read the file");
            return;
        }
        serve_static(fd, filename, sbuf.st_size);
    }
    else {  // serve dynamic contant
        if(!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) {
            clienterror(fd, filename, "403", "Forbidden", "Tiny could't run the CGI program");
            return;
        }
        serve_dynamic(fd, filename, cgiargs);
    }
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

void read_requesthdrs(rio_t *rp) {
    char buf[MAXLINE];

    Rio_readlineb(rp, buf, MAXLINE);
    while(strcmp(buf, "\r\n")) {
        Rio_readlineb(rp, buf, MAXLINE);
        printf("%s", buf);
    }
    return;
}

int parse_uri(char *uri, char *filename, char *cgiargs) {
    char *ptr;

    if(!strstr(uri, "cgi-bin")) {  // Static content
        strcpy(cgiargs, "");
        strcpy(filename, "./");

        if(uri[strlen(uri)-1] == '/')
            strcat(filename, "home.html");
        else {
            for(char *temp = uri; temp != NULL; temp = strchr(temp, '/')) 
                ptr = ++temp;
            strcat(filename, ptr);
        }

        return 1;
    }

    else {  // Dynamic content
        ptr = strchr(uri, '?');
        if(ptr) {
            strcpy(cgiargs, ptr + 1);
            *ptr = '\0';
        }

        else
            strcpy(cgiargs, "");
        strcpy(filename, ".");
        strcat(filename, uri);
        return 0;
    }
}

void serve_static(int fd, char *filename, int filesize) {
    int srcfd;
    char *srcp, filetype[MAXLINE], buf[MAXLINE];

    // Send response headers to client
    get_filetype(filename, filetype);
    int idx = 0;
    idx += sprintf(buf + idx, "HTTP/1.1 200 OK\r\n");
    idx += sprintf(buf + idx, "Server: Tiny Web Server\r\n");
    idx += sprintf(buf + idx, "Connection: keep-alive\r\n");
    idx += sprintf(buf + idx, "Content-length: %d\r\n", filesize);
    idx += sprintf(buf + idx, "Content-type: %s\r\n\r\n", filetype);
    Rio_writen(fd, buf, strlen(buf));
    printf("Response headers:\n");
    printf("%s", buf);

    // Send response body to client
    srcfd = Open(filename, O_RDONLY, 0);
    char* temp = (char*)malloc(filesize);
    Rio_readn(srcfd, temp, filesize);
    Close(srcfd);
    Rio_writen(fd, temp, filesize);
    free(temp);
}

void get_filetype(char *filename, char *filetype) {
    if(strstr(filename, ".html"))
        strcpy(filetype, "text/html");
    else if(strstr(filename, ".gif"))
        strcpy(filetype, "image/gif");
    else if(strstr(filename, ".png"))
        strcpy(filetype, "image/png");
    else if(strstr(filename, ".jpg"))
        strcpy(filetype, "image/jpeg");
    else if(strstr(filename, ".mpg"))
        strcpy(filetype, "vedio/mpeg");
    else if(strstr(filename, ".mp4"))
        strcpy(filetype, "vedio/mp4");
    else 
        strcpy(filetype, "text/plain");
}

void serve_dynamic(int fd, char *filename, char *cgiargs) {
    char buf[MAXLINE], *emptylist[] = { NULL };

    // Return first part of HTTP response
    sprintf(buf, "HTTP/1.1 200 OK\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Server: Tiny Web Server\r\n");
    Rio_writen(fd, buf, strlen(buf));

    if(Fork() == 0) {  // Child
        // Real server would set all CGI vars here
        setenv("QUERY_STRING", cgiargs, 1);
        Dup2(fd, STDOUT_FILENO);  // Redirect stdout to client
        Execve(filename, emptylist, environ);  // Run CGI program
    }
    Wait(NULL);  // Parent waits for and reaps child
}