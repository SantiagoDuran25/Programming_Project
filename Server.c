// server.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <strings.h>   // for strcasecmp

#include "Common.h"
#include "Data_Store.h"

typedef struct {
    int sockfd;
    int active;
} Client;

Client clients[MAX_CLIENTS];
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

void die(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

void clients_lock(void)   { pthread_mutex_lock(&clients_mutex); }
void clients_unlock(void) { pthread_mutex_unlock(&clients_mutex); }

Client *add_client(int sockfd) {
    Client *res = NULL;
    clients_lock();
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (!clients[i].active) {
            clients[i].active = 1;
            clients[i].sockfd = sockfd;
            res = &clients[i];
            break;
        }
    }
    clients_unlock();
    return res;
}

void remove_client(int sockfd) {
    clients_lock();
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active && clients[i].sockfd == sockfd) {
            clients[i].active = 0;
            clients[i].sockfd = -1;
            break;
        }
    }
    clients_unlock();
}

int recv_line(int sockfd, char *buf, size_t size) {
    // simple line read: read until '\n' or buffer full
    size_t pos = 0;
    while (pos + 1 < size) {
        char c;
        ssize_t n = recv(sockfd, &c, 1, 0);
        if (n <= 0) {
            if (pos == 0) return -1;
            break;
        }
        if (c == '\n') {
            break;
        }
        buf[pos++] = c;
    }
    buf[pos] = '\0';
    return (int)pos;
}

void trim(char *s) {
    // remove leading/trailing spaces
    char *start = s;
    while (*start == ' ' || *start == '\t') start++;
    if (start != s) memmove(s, start, strlen(start) + 1);

    size_t len = strlen(s);
    while (len > 0 && (s[len-1] == ' ' || s[len-1] == '\t' ||
                       s[len-1] == '\r')) {
        s[--len] = '\0';
    }
}

void handle_command(int sockfd, char *line) {
    char *saveptr;
    char *cmd = strtok_r(line, " ", &saveptr);
    if (!cmd) {
        send(sockfd, "ERROR Empty command\n", 21, 0);
        return;
    }

    // SET key value...
    if (strcasecmp(cmd, "SET") == 0) {
        char *key = strtok_r(NULL, " ", &saveptr);
        char *value = saveptr;

        if (!key || !value) {
            const char *msg = "ERROR Usage: SET <key> <value>\n";
            send(sockfd, msg, strlen(msg), 0);
            return;
        }

        trim(key);
        trim(value);

        if (strlen(key) >= KEY_LEN || strlen(value) >= VALUE_LEN) {
            const char *msg = "ERROR key/value too long\n";
            send(sockfd, msg, strlen(msg), 0);
            return;
        }

        if (kv_set(key, value) == 0) {
            const char *msg = "OK\n";
            send(sockfd, msg, strlen(msg), 0);
        } else {
            const char *msg = "ERROR internal set failed\n";
            send(sockfd, msg, strlen(msg), 0);
        }
        return;
    }

    // GET key
    if (strcasecmp(cmd, "GET") == 0) {
        char *key = strtok_r(NULL, " ", &saveptr);
        if (!key) {
            const char *msg = "ERROR Usage: GET <key>\n";
            send(sockfd, msg, strlen(msg), 0);
            return;
        }
        trim(key);

        char value[VALUE_LEN];
        if (kv_get(key, value, sizeof(value)) == 0) {
            char out[BUF_SIZE];
            snprintf(out, sizeof(out), "VALUE %s\n", value);
            send(sockfd, out, strlen(out), 0);
        } else {
            const char *msg = "NOTFOUND\n";
            send(sockfd, msg, strlen(msg), 0);
        }
        return;
    }

    // DEL key
    if (strcasecmp(cmd, "DEL") == 0) {
        char *key = strtok_r(NULL, " ", &saveptr);
        if (!key) {
            const char *msg = "ERROR Usage: DEL <key>\n";
            send(sockfd, msg, strlen(msg), 0);
            return;
        }
        trim(key);

        if (kv_del(key) == 0) {
            const char *msg = "DELETED\n";
            send(sockfd, msg, strlen(msg), 0);
        } else {
            const char *msg = "NOTFOUND\n";
            send(sockfd, msg, strlen(msg), 0);
        }
        return;
    }

    // KEYS
    if (strcasecmp(cmd, "KEYS") == 0) {
        char keys_buf[BUF_SIZE];
        int count = kv_keys(keys_buf, sizeof(keys_buf));
        char out[BUF_SIZE + 64];

        if (count == 0) {
            snprintf(out, sizeof(out), "KEYS 0\n");
        } else {
            snprintf(out, sizeof(out), "KEYS %d %s\n", count, keys_buf);
        }
        send(sockfd, out, strlen(out), 0);
        return;
    }

    // SAVE
    if (strcasecmp(cmd, "SAVE") == 0) {
        if (kv_save() == 0) {
            const char *msg = "OK\n";
            send(sockfd, msg, strlen(msg), 0);
        } else {
            const char *msg = "ERROR save failed\n";
            send(sockfd, msg, strlen(msg), 0);
        }
        return;
    }

    // EXIT handled in thread loop, but we can reply anyway
    if (strcasecmp(cmd, "EXIT") == 0) {
        const char *msg = "BYE\n";
        send(sockfd, msg, strlen(msg), 0);
        return;
    }

    const char *msg = "ERROR Unknown command. Use SET, GET, DEL, KEYS, SAVE, EXIT\n";
    send(sockfd, msg, strlen(msg), 0);
}

void *client_thread(void *arg) {
    Client *cli = (Client *)arg;
    int sockfd = cli->sockfd;

    const char *hello =
        "Welcome to TinyKV.\n"
        "Commands:\n"
        "  SET <key> <value>\n"
        "  GET <key>\n"
        "  DEL <key>\n"
        "  KEYS\n"
        "  SAVE\n"
        "  EXIT\n\n";
    send(sockfd, hello, strlen(hello), 0);

    char line[BUF_SIZE];

    while (1) {
        int n = recv_line(sockfd, line, sizeof(line));
        if (n <= 0) {
            break; // client disconnected
        }
        trim(line);
        if (line[0] == '\0') continue;

        if (strcasecmp(line, "EXIT") == 0) {
            const char *msg = "BYE\n";
            send(sockfd, msg, strlen(msg), 0);
            break;
        }

        handle_command(sockfd, line);
    }

    close(sockfd);
    remove_client(sockfd);
    pthread_detach(pthread_self());
    return NULL;
}

int main(int argc, char *argv[]) {
    int port = (argc >= 2) ? atoi(argv[1]) : DEFAULT_PORT;
    if (port <= 0) port = DEFAULT_PORT;

    // init clients array
    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].active = 0;
        clients[i].sockfd = -1;
    }

    kv_init(DUMP_FILE);

    int listen_fd;
    if ((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        die("socket");

    int opt = 1;
    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
        die("setsockopt");

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family      = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port        = htons(port);

    if (bind(listen_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
        die("bind");

    if (listen(listen_fd, 8) == -1)
        die("listen");

    printf("TinyKV server listening on port %d\n", port);

    while (1) {
        struct sockaddr_in cli_addr;
        socklen_t cli_len = sizeof(cli_addr);
        int client_fd = accept(listen_fd, (struct sockaddr *)&cli_addr, &cli_len);
        if (client_fd == -1) {
            perror("accept");
            continue;
        }

        Client *cli = add_client(client_fd);
        if (!cli) {
            const char *msg = "ERROR Server full\n";
            send(client_fd, msg, strlen(msg), 0);
            close(client_fd);
            continue;
        }

        pthread_t tid;
        if (pthread_create(&tid, NULL, client_thread, cli) != 0) {
            perror("pthread_create");
            close(client_fd);
            remove_client(client_fd);
        }
    }

    close(listen_fd);
    kv_shutdown();
    return 0;
}

