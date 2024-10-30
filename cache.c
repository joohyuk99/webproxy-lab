#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

extern void parseURI(char *uri, char *hostname, char *port);

sem_t sem2;  // semaphore for caching

// cache binary search tree
typedef struct _cacheNode{
    char uri[MAXLINE];
    char header[MAXLINE];
    char content[MAX_OBJECT_SIZE];
    int contentSize;
    struct _cacheNode *next;
    struct _cacheNode *prev;
} cacheNode;

cacheNode *start;
cacheNode *end;

void initCache() {
    start = end = NULL;
}

// if request is cached, response to client from cache and return 1
// else return 0
int checkCached(int clientfd, char *method, char *uri) {
    cacheNode *currentNode = start;
    int compareURI;
    while(currentNode != NULL) {
        printf("👌 current cache: %s, %ld\n", currentNode->uri, currentNode->contentSize);
        compareURI = !strcmp(uri, currentNode->uri);
        if(compareURI) {  // if cached
            Rio_writen(clientfd, currentNode->header, strlen(currentNode->header));
            printf("cached header:\n");
            printf("%s", currentNode->header);
            Rio_writen(clientfd, currentNode->content, currentNode->contentSize);
            return 1;
        }
        else {
            printf("current uri: %s\n", currentNode->uri);
            currentNode = currentNode->next;
        }
    }
    return 0;
}

// caching response and response to client
void cached(int clientfd, char *uri, char *header, char *content, int contentSize) {

    if(MAX_OBJECT_SIZE < contentSize) {
        Rio_writen(clientfd, header, strlen(header));
        Rio_writen(clientfd, content, contentSize);
        return;
    }

    cacheNode *new = (cacheNode*)Malloc(sizeof(cacheNode));
    new->prev = end;
    if(start == NULL)
        start = end = new;
    else {
        end->next = new;
        end = new;
    }
    new->next = NULL;  // init new node
    printf("caching header:\n");
    printf("%s", header);
    printf("caching header end\n");
    strcpy(new->uri, uri);
    strcpy(new->header, header);
    strcpy(new->content, content);
    new->contentSize = contentSize;

    printf("cache header:\n");
    printf("%s", header);

    if(start == NULL) {
        start = end = new;
    }
    else {
        end->next = new;
        new->prev = end;
        end = end->next;
    }
    start->prev = end->next = NULL;

    Rio_writen(clientfd, header, strlen(header));
    Rio_writen(clientfd, content, contentSize);

    printf("cache\n");
    printf("%s\n", start->uri);
    printf("%ld\n", start->contentSize);
    if(start->next == NULL)
        printf("NULL\n");
    else {
        printf("%s\n", start->next->uri);
        printf("%d\n", start->next->contentSize);
        if(start->next->next == NULL)
            printf("NULL\n");
        else {
            printf("%s\n", start->next->next->uri);
            printf("%d\n", start->next->next->contentSize);
        }
    }

    return;
}