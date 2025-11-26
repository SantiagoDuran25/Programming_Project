// kvstore.c
#include "kvstore.h"
#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

typedef struct Entry {
    char key[KEY_LEN];
    char value[VALUE_LEN];
    struct Entry *next;
} Entry;

static Entry *table[HASH_SIZE];
static pthread_mutex_t kv_mutex = PTHREAD_MUTEX_INITIALIZER;
static char dump_filename[256] = DUMP_FILE;

/* ---------- helpers ---------- */

static unsigned int hash_key(const char *key) {
    unsigned int h = 5381;
    int c;
    while ((c = (unsigned char)*key++)) {
        h = ((h << 5) + h) + c;
    }
    return h % HASH_SIZE;
}

static void kv_lock(void)   { pthread_mutex_lock(&kv_mutex); }
static void kv_unlock(void) { pthread_mutex_unlock(&kv_mutex); }

static void free_table(void) {
    for (int i = 0; i < HASH_SIZE; i++) {
        Entry *e = table[i];
        while (e) {
            Entry *next = e->next;
            free(e);
            e = next;
        }
        table[i] = NULL;
    }
}

/* internal set without locking (assumes mutex held) */
static int kv_set_internal(const char *key, const char *value) {
    if (!key || !value) return -1;

    unsigned int h = hash_key(key);
    Entry *e = table[h];

    while (e) {
        if (strncmp(e->key, key, KEY_LEN) == 0) {
            strncpy(e->value, value, VALUE_LEN - 1);
            e->value[VALUE_LEN - 1] = '\0';
            return 0;
        }
        e = e->next;
    }

    e = (Entry *)malloc(sizeof(Entry));
    if (!e) return -1;
    strncpy(e->key, key, KEY_LEN - 1);
    e->key[KEY_LEN - 1] = '\0';
    strncpy(e->value, value, VALUE_LEN - 1);
    e->value[VALUE_LEN - 1] = '\0';
    e->next = table[h];
    table[h] = e;
    return 0;
}

/* ---------- public API ---------- */

void kv_init(const char *filename) {
    if (filename && filename[0] != '\0') {
        strncpy(dump_filename, filename, sizeof(dump_filename) - 1);
        dump_filename[sizeof(dump_filename) - 1] = '\0';
    }

    for (int i = 0; i < HASH_SIZE; i++) {
        table[i] = NULL;
    }

    // load from file if exists
    FILE *f = fopen(dump_filename, "r");
    if (!f) {
        return; // ok, fresh db
    }

    char line[KEY_LEN + VALUE_LEN + 8];
    while (fgets(line, sizeof(line), f)) {
        // format: key\tvalue\n
        char *tab = strchr(line, '\t');
        if (!tab) continue;
        *tab = '\0';
        char *key = line;
        char *value = tab + 1;

        // strip newline from value
        size_t len = strlen(value);
        while (len > 0 && (value[len-1] == '\n' || value[len-1] == '\r')) {
            value[--len] = '\0';
        }

        kv_lock();
        kv_set_internal(key, value);
        kv_unlock();
    }

    fclose(f);
}

void kv_shutdown(void) {
    kv_save();
    kv_lock();
    free_table();
    kv_unlock();
}

int kv_set(const char *key, const char *value) {
    if (!key || !value) return -1;
    kv_lock();
    int r = kv_set_internal(key, value);
    kv_unlock();
    return r;
}

int kv_get(const char *key, char *out_value, size_t out_size) {
    if (!key || !out_value || out_size == 0) return -1;

    kv_lock();
    unsigned int h = hash_key(key);
    Entry *e = table[h];
    while (e) {
        if (strncmp(e->key, key, KEY_LEN) == 0) {
            strncpy(out_value, e->value, out_size - 1);
            out_value[out_size - 1] = '\0';
            kv_unlock();
            return 0;
        }
        e = e->next;
    }
    kv_unlock();
    return -1;
}

int kv_del(const char *key) {
    if (!key) return -1;

    kv_lock();
    unsigned int h = hash_key(key);
    Entry *e = table[h];
    Entry *prev = NULL;

    while (e) {
        if (strncmp(e->key, key, KEY_LEN) == 0) {
            if (prev) prev->next = e->next;
            else       table[h] = e->next;
            free(e);
            kv_unlock();
            return 0;
        }
        prev = e;
        e = e->next;
    }

    kv_unlock();
    return -1;
}

int kv_keys(char *out_buf, size_t out_size) {
    if (!out_buf || out_size == 0) return 0;

    out_buf[0] = '\0';
    size_t used = 0;
    int count = 0;

    kv_lock();
    for (int i = 0; i < HASH_SIZE; i++) {
        for (Entry *e = table[i]; e != NULL; e = e->next) {
            size_t klen = strnlen(e->key, KEY_LEN);
            if (used + klen + 2 >= out_size) { // +1 space +1 \0
                kv_unlock();
                return count;
            }
            memcpy(out_buf + used, e->key, klen);
            used += klen;
            out_buf[used++] = ' ';
            out_buf[used] = '\0';
            count++;
        }
    }
    kv_unlock();

    if (used > 0 && out_buf[used-1] == ' ') {
        out_buf[used-1] = '\0';
    }
    return count;
}

int kv_save(void) {
    kv_lock();
    FILE *f = fopen(dump_filename, "w");
    if (!f) {
        kv_unlock();
        return -1;
    }

    for (int i = 0; i < HASH_SIZE; i++) {
        for (Entry *e = table[i]; e != NULL; e = e->next) {
            // forbid newlines/tabs in value for simplicity
            fprintf(f, "%s\t%s\n", e->key, e->value);
        }
    }

    fclose(f);
    kv_unlock();
    return 0;
}

