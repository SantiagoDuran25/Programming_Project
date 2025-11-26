// client.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>

#include "common.h"

void die(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

int recv_line(int sockfd, char *buf, size_t size) {
    size_t pos = 0;
    while (pos + 1 < size) {
        char c;
        ssize_t n = recv(sockfd, &c, 1, 0);
        if (n <= 0) {
            if (pos == 0) return -1;
            break;
        }
        if (c == '\n') break;
        buf[pos++] = c;
    }
    buf[pos] = '\0';
    return (int)pos;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <server_ip> <port>\n", argv[0]);
        return 1;
    }

    const char *server_ip = argv[1];
    int port = atoi(argv[2]);
    if (port <= 0) port = DEFAULT_PORT;

    int sockfd;
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        die("socket");

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port   = htons(port);

    if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0)
        die("inet_pton");

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
        die("connect");

    printf("Connected to TinyKV at %s:%d\n", server_ip, port);

    // Read initial welcome message (a few lines)
    char line[BUF_SIZE];
    int n;
    while ((n = recv_line(sockfd, line, sizeof(line))) > 0) {
        if (strlen(line) == 0) break;
        printf("%s\n", line);
        if (strstr(line, "EXIT") != NULL) {
            // assume help finished
            break;
        }
    }

    char buf[BUF_SIZE];
    while (1) {
        printf("TinyKV> ");
        fflush(stdout);

        if (!fgets(buf, sizeof(buf), stdin)) {
            break;
        }

        size_t len = strlen(buf);
        if (len == 0) continue;
        if (buf[len-1] != '\n') {
            // line too long; flush stdin
            int c;
            while ((c = getchar()) != '\n' && c != EOF) {}
        }

        if (send(sockfd, buf, strlen(buf), 0) == -1) {
            perror("send");
            break;
        }

        // if user typed EXIT, server will close after reply
        if (strncasecmp(buf, "EXIT", 4) == 0) {
            n = recv_line(sockfd, line, sizeof(line));
            if (n > 0) {
                printf("%s\n", line);
            }
            break;
        }

        // read 1 response line (simple protocol)
        n = recv_line(sockfd, line, sizeof(line));
        if (n <= 0) {
            printf("Disconnected from server.\n");
            break;
        }
        printf("%s\n", line);
    }

    close(sockfd);
    printf("Client exiting.\n");
    return 0;
}
