#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#include "Common.h"
#include "Data_Store.h"

void *client_handler(void *arg) {
    int sockfd = *(int *)arg;
    free(arg);

    char buffer[512];
    char cmd[16];
    char key[KEY_LEN];
    char value[VALUE_LEN];

    while (1) {
        ssize_t n = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
        if (n <= 0) {
            break;  // client disconnected
        }
        buffer[n] = '\0';

        // Parse command word
        if (sscanf(buffer, "%15s", cmd) != 1) {
            send(sockfd, "ERROR\n", 6, 0);
            continue;
        }

        // ================= EXIT =================
        if (!strcasecmp(cmd, "EXIT")) {
            send(sockfd, "Bye\n", 4, 0);
            break;
        }

        // ================= SET ==================
        else if (!strcasecmp(cmd, "SET")) {
            // key is first word after SET, value is rest of line
            char *p = buffer + 3;          // after "SET"
            while (*p == ' ') p++;         // skip spaces

            if (sscanf(p, "%63s", key) != 1) {
                send(sockfd, "ERROR\n", 6, 0);
                continue;
            }

            char *v = strchr(p, ' ');
            if (!v) {
                send(sockfd, "ERROR\n", 6, 0);
                continue;
            }
            v++;  // first char of value

            // copy entire remaining line as value
            strncpy(value, v, VALUE_LEN);
            value[VALUE_LEN - 1] = '\0';
            // strip trailing newline / \r
            value[strcspn(value, "\r\n")] = '\0';

            kv_set(key, value);
            send(sockfd, "OK\n", 3, 0);
        }

        // ================= GET ==================
        else if (!strcasecmp(cmd, "GET")) {
            if (sscanf(buffer, "%*s %63s", key) == 1) {
                if (kv_get(key, value, sizeof(value)) == 0) {
                    char out[VALUE_LEN + 2];
                    snprintf(out, sizeof(out), "%s\n", value);
                    send(sockfd, out, strlen(out), 0);
                } else {
                    send(sockfd, "NOT FOUND\n", 10, 0);
                }
            } else {
                send(sockfd, "ERROR\n", 6, 0);
            }
        }

        // ================= DEL ==================
        else if (!strcasecmp(cmd, "DEL")) {
            if (sscanf(buffer, "%*s %63s", key) == 1) {
                if (kv_delete(key) == 0) {
                    send(sockfd, "DELETED\n", 8, 0);
                } else {
                    send(sockfd, "NOT FOUND\n", 10, 0);
                }
            } else {
                send(sockfd, "ERROR\n", 6, 0);
            }
        }

        // ================= KEYS =================
        else if (!strcasecmp(cmd, "KEYS")) {
            char keys_buf[4096];
            int used = kv_keys(keys_buf, sizeof(keys_buf));
            // kv_keys should always write something (even "(empty)\n")
            send(sockfd, keys_buf, used, 0);
        }

        // ================= SAVE =================
        else if (!strcasecmp(cmd, "SAVE")) {
            if (kv_save() == 0)
                send(sockfd, "SAVED\n", 6, 0);
            else
                send(sockfd, "ERROR\n", 6, 0);
        }

        // ============== UNKNOWN =================
        else {
            send(sockfd, "UNKNOWN\n", 8, 0);
        }
    }

    close(sockfd);
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    int port = 5001;
    if (argc == 2) {
        port = atoi(argv[1]);
    }

    // Initialize key-value store from file
    kv_init(DUMP_FILE);

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        exit(1);
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        exit(1);
    }

    if (listen(server_fd, 5) < 0) {
        perror("listen");
        exit(1);
    }

    printf("TinyKV server listening on port %d\n", port);

    while (1) {
        int client_fd = accept(server_fd, NULL, NULL);
        if (client_fd < 0) {
            perror("accept");
            continue;
        }

        int *pclient = malloc(sizeof(int));
        *pclient = client_fd;

        pthread_t tid;
        pthread_create(&tid, NULL, client_handler, pclient);
        pthread_detach(tid);
    }

    close(server_fd);
    return 0;
}
