#include "csapp.h"

int main() {
    char *buf, *p;
    char arg1[MAXLINE], arg2[MAXLINE], content[MAXLINE];
    int n1 = 0, n2 = 0;

    // Extract two arguments
    if((buf = getenv("QUERY_STRING")) != NULL) {
        p = strchr(buf, '&');
        *p = '\0';

        strcpy(arg1, buf + 3);
        strcpy(arg2, p + 4);

        n1 = atoi(arg1);
        n2 = atoi(arg2);
    }

    // Make response body (HTML)
    int idx = 0;
    idx += sprintf(content + idx, "QUERY_STRING = %s&%s<br/>", buf, p + 1);
    idx += sprintf(content + idx, "Welcome to add.com<br/>");
    idx += sprintf(content + idx, "The Internet Additional Protal\r\n");
    idx += sprintf(content + idx, "<p>The Answer is %d + %d = %d\r\n<p>", n1, n2, n1 + n2);
    idx += sprintf(content + idx, "Thanks for visiting!\r\n");

    // Generate the HTTP response
    printf("Connection: close\r\n");
    printf("Content-length: %d\r\n", (int)strlen(content));
    printf("Content-type: text/html\r\n\r\n");
    printf("%s", content);
    fflush(stdout);

    exit(0);
}