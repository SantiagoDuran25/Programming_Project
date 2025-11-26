#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#include "Common.h"
#include "Data_Store.h"

// =======================
//   CLIENT HANDLER
// =======================
void *client_handler(void *arg) {
    int sockfd = *(int*)arg;
    free(arg);

    char buffer[512];
    char cmd[16], key[KEY_LEN], value[VALUE_LEN];

    while (1) {
        memset(buffer, 0, sizeof(buffer));
        ssize_t n = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
        if (n <= 0) break;
        buffer[n] = '\0';

        // Trim leading whitespace
        char *p = buffer;
        while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') p++;
        if (*p == '\0') {  // Blank input
            send(sockfd, "> ", 2, 0);
            continue;
        }

        // Extract command
        if (sscanf(p, "%15s", cmd) != 1) {
            send(sockfd, "ERROR: Invalid input\n> ", 22, 0);
            continue;
        }

        // ================= EXIT =================
        if (!strcasecmp(cmd, "EXIT")) {
            send(sockfd, "Bye\n", 4, 0);
            break;
        }

        // ================= SET ==================
        else if (!strcasecmp(cmd, "SET")) {
            char *args = p + 3;  // skip "SET"
            while (*args == ' ') args++;

            if (sscanf(args, "%63s", key) != 1) {
                send(sockfd, "ERROR: Usage SET <key> <value>\n> ", 33, 0);
                continue;
            }

            char *v = strchr(args, ' ');
            if (!v) {
                send(sockfd, "ERROR: Usage SET <key> <value>\n> ", 33, 0);
                continue;
            }

            v++;
            while (*v == ' ') v++;

            if (*v == '\0') {
                send(sockfd, "ERROR: Value cannot be empty\n> ", 30, 0);
                continue;
            }

            // Check for extra arguments beyond value
            char extra[8];
            if (sscanf(v, "%255s %7s", value, extra) == 2) {
                send(sockfd, "ERROR: Too many arguments\n> ", 28, 0);
                continue;
            }

            kv_set(key, v);
            send(sockfd, "OK\n> ", 5, 0);
        }

        // ================= GET ==================
        else if (!strcasecmp(cmd, "GET")) {
            if (sscanf(p, "%*s %63s", key) != 1) {
                send(sockfd, "ERROR: Usage GET <key>\n> ", 24, 0);
                continue;
            }

            if (kv_get(key, value, sizeof(value)) == 0) {
                char out[VALUE_LEN + 4];
                snprintf(out, sizeof(out), "%s\n> ", value);
                send(sockfd, out, strlen(out), 0);
            } else {
                send(sockfd, "NOT FOUND\n> ", 12, 0);
            }
        }

        // ================= DEL ==================
        else if (!strcasecmp(cmd, "DEL")) {
            if (sscanf(p, "%*s %63s", key) != 1) {
                send(sockfd, "ERROR: Usage DEL <key>\n> ", 24, 0);
                continue;
            }
            if (kv_delete(key) == 0)
                send(sockfd, "DELETED\n> ", 10, 0);
            else
                send(sockfd, "NOT FOUND\n> ", 12, 0);
        }

        // ================= KEYS =================
        else if (!strcasecmp(cmd, "KEYS")) {
            char keys_buf[4096];
            int used = kv_keys(keys_buf, sizeof(keys_buf));
            send(sockfd, keys_buf, used, 0);
            send(sockfd, "> ", 2, 0);
        }

        // ================= SAVE =================
        else if (!strcasecmp(cmd, "SAVE")) {
            if (kv_save() == 0)
                send(sockfd, "SAVED\n> ", 8, 0);
            else
                send(sockfd, "ERROR: Save failed\n> ", 21, 0);
        }

        // ============== UNKNOWN COMMAND ==========
        else {
            send(sockfd, "ERROR: Unknown command\n> ", 25, 0);
        }
    }

    close(sockfd);
    pthread_exit(NULL);
}

// =======================
//   SERVER MAIN
// =======================
int main(int argc, char *argv[]) {
    int port = 5001;
    if (argc == 2) port = atoi(argv[1]);

    // Load existing data
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
