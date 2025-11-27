#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "Common.h"
#include "Data_Store.h"

// =======================
//   ENTRY STRUCT
// =======================
typedef struct Entry {
    char key[KEY_LEN];
    char value[VALUE_LEN];
    struct Entry *next;
} Entry;

// =======================
//   GLOBALS
// =======================
static Entry *table[HASH_SIZE];
static char dump_filename[64];

// Mutex for thread safety
pthread_mutex_t kv_lock = PTHREAD_MUTEX_INITIALIZER;

// =======================
//   HASH FUNCTION
// =======================
static unsigned int hash(const char *key) {
    unsigned int h = 0;
    while (*key) h = (h * 31) + (unsigned char)(*key++);
    return h % HASH_SIZE;
}

// =======================
//   INITIALIZE STORE
// =======================
int kv_init(const char *filename) {
    memset(table, 0, sizeof(table));

    // Store dump filename safely
    strncpy(dump_filename, filename, sizeof(dump_filename) - 1);
    dump_filename[sizeof(dump_filename) - 1] = '\0';

    FILE *fp = fopen(filename, "r");
    if (!fp) return 0; // No file yet

    char key[KEY_LEN], value[VALUE_LEN];
    while (fscanf(fp, "%63[^=]=%255s\n", key, value) == 2) {
        kv_set(key, value);
    }
    fclose(fp);
    return 0;
}

// =======================
//   SET
// =======================
int kv_set(const char *key, const char *value) {
    pthread_mutex_lock(&kv_lock);

    unsigned int h = hash(key);
    Entry *curr = table[h];

    // Overwrite existing key
    while (curr) {
        if (strcmp(curr->key, key) == 0) {
            strncpy(curr->value, value, VALUE_LEN - 1);
            curr->value[VALUE_LEN - 1] = '\0'; // ensure null-termination
            pthread_mutex_unlock(&kv_lock);
            return 0;
        }
        curr = curr->next;
    }

    // Create new entry
    curr = malloc(sizeof(Entry));
    if (!curr) {
        pthread_mutex_unlock(&kv_lock);
        return -1;
    }

    strncpy(curr->key, key, KEY_LEN - 1);
    curr->key[KEY_LEN - 1] = '\0';

    strncpy(curr->value, value, VALUE_LEN - 1);
    curr->value[VALUE_LEN - 1] = '\0';

    curr->next = table[h];
    table[h] = curr;

    pthread_mutex_unlock(&kv_lock);
    return 0;
}

// =======================
//   GET
// =======================
int kv_get(const char *key, char *out_buf, size_t out_size) {
    pthread_mutex_lock(&kv_lock);

    unsigned int h = hash(key);
    Entry *curr = table[h];

    while (curr) {
        if (strcmp(curr->key, key) == 0) {
            if (out_size == 0) {
                pthread_mutex_unlock(&kv_lock);
                return -1;
            }
            // Copy at most out_size-1 characters and always terminate
            strncpy(out_buf, curr->value, out_size - 1);
            out_buf[out_size - 1] = '\0';
            pthread_mutex_unlock(&kv_lock);
            return 0;
        }
        curr = curr->next;
    }

    pthread_mutex_unlock(&kv_lock);
    return -1;
}

// =======================
//   DELETE
// =======================
int kv_delete(const char *key) {
    pthread_mutex_lock(&kv_lock);

    unsigned int h = hash(key);
    Entry *curr = table[h];
    Entry *prev = NULL;

    while (curr) {
        if (strcmp(curr->key, key) == 0) {
            if (prev) prev->next = curr->next;
            else table[h] = curr->next;
            free(curr);
            pthread_mutex_unlock(&kv_lock);
            return 0;
        }
        prev = curr;
        curr = curr->next;
    }

    pthread_mutex_unlock(&kv_lock);
    return -1;
}

// =======================
//   LIST KEYS
// =======================
int kv_keys(char *out_buf, size_t out_size) {
    pthread_mutex_lock(&kv_lock);

    size_t used = 0;

    for (int i = 0; i < HASH_SIZE; i++) {
        Entry *curr = table[i];
        while (curr) {
            int written = snprintf(out_buf + used, out_size - used, "%s\n", curr->key);
            if (written < 0 || written >= (int)(out_size - used)) {
                pthread_mutex_unlock(&kv_lock);
                return used;
            }
            used += written;
            curr = curr->next;
        }
    }

    if (used == 0) {
        snprintf(out_buf, out_size, "(empty)\n");
        used = strlen(out_buf);
    }

    pthread_mutex_unlock(&kv_lock);
    return used;
}

// =======================
//   SAVE TO DISK
// =======================
int kv_save(void) {
    pthread_mutex_lock(&kv_lock);

    FILE *fp = fopen(dump_filename, "w");
    if (!fp) {
        pthread_mutex_unlock(&kv_lock);
        return -1;
    }

    for (int i = 0; i < HASH_SIZE; i++) {
        Entry *curr = table[i];
        while (curr) {
            fprintf(fp, "%s=%s\n", curr->key, curr->value);
            curr = curr->next;
        }
    }

    fclose(fp);
    pthread_mutex_unlock(&kv_lock);
    return 0;
}
