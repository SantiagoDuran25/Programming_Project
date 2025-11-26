#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define BUF_SIZE 512

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <server_ip> <port>\n", argv[0]);
        exit(1);
    }

    int sockfd;
    struct sockaddr_in serv_addr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        exit(1);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[2]));
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);

    if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("connect");
        exit(1);
    }

    printf("Welcome to TinyKV.\n");
    printf("Commands:\n");
    printf(" SET <key> <value>\n GET <key>\n DEL <key>\n KEYS\n SAVE\n EXIT\n");

    // Print prompt ONLY if interactive terminal
    if (isatty(STDIN_FILENO)) {
        printf("> ");
        fflush(stdout);
    }

    char send_buf[BUF_SIZE];
    char recv_buf[BUF_SIZE];

    while (1) {
        if (!fgets(send_buf, sizeof(send_buf), stdin))
            break;

        // Send to server
        send(sockfd, send_buf, strlen(send_buf), 0);

        // If EXIT command, break right away
        if (strncasecmp(send_buf, "EXIT", 4) == 0)
            break;

        // Receive server response
        int n = recv(sockfd, recv_buf, sizeof(recv_buf) - 1, 0);
        if (n <= 0) break;
        recv_buf[n] = '\0';

        // Print server response
        printf("%s", recv_buf);

        // Print prompt ONLY if interactive mode
        if (isatty(STDIN_FILENO)) {
            printf("> ");
            fflush(stdout);
        }
    }

    close(sockfd);
    return 0;
}
