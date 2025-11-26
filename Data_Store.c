#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Common.h"
#include "Data_Store.h"

// =======================
//   ENTRY STRUCT (Linked List)
// =======================
typedef struct Entry {
    char key[KEY_LEN];
    char value[VALUE_LEN];
    struct Entry *next;
} Entry;

// =======================
//   GLOBAL HASH TABLE & DUMP FILE
// =======================
static Entry *table[HASH_SIZE];
static char dump_filename[64];

// =======================
//   SIMPLE HASH FUNCTION
// =======================
static unsigned int hash(const char *key) {
    unsigned int h = 0;
    while (*key) h = (h * 31) + (unsigned char)(*key++);
    return h % HASH_SIZE;
}

// =======================
//   INITIALIZE DATA STORE
// =======================
int kv_init(const char *filename) {
    // IMPORTANT: reset table completely
    memset(table, 0, sizeof(table));

    strncpy(dump_filename, filename, sizeof(dump_filename));

    FILE *fp = fopen(filename, "r");
    if (!fp) return 0; // File may not exist (fresh start)

    char key[KEY_LEN], value[VALUE_LEN];
    while (fscanf(fp, "%63[^=]=%255s\n", key, value) == 2) {
        kv_set(key, value);
    }
    fclose(fp);
    return 0;
}

// =======================
//   SET KEY-VALUE PAIR
// =======================
int kv_set(const char *key, const char *value) {
    unsigned int h = hash(key);
    Entry *curr = table[h];

    while (curr) {
        if (strcmp(curr->key, key) == 0) {
            strncpy(curr->value, value, VALUE_LEN);
            return 0;
        }
        curr = curr->next;
    }

    // Create new entry
    curr = malloc(sizeof(Entry));
    strncpy(curr->key, key, KEY_LEN);
    strncpy(curr->value, value, VALUE_LEN);
    curr->next = table[h];
    table[h] = curr;
    return 0;
}

// =======================
//   GET VALUE FOR KEY
// =======================
int kv_get(const char *key, char *out_buf, size_t out_size) {
    unsigned int h = hash(key);
    Entry *curr = table[h];

    while (curr) {
        if (strcmp(curr->key, key) == 0) {
            strncpy(out_buf, curr->value, out_size);
            return 0;
        }
        curr = curr->next;
    }
    return -1;
}

// =======================
//   DELETE KEY-VALUE PAIR
// =======================
int kv_delete(const char *key) {
    unsigned int h = hash(key);
    Entry *curr = table[h];
    Entry *prev = NULL;

    while (curr) {
        if (strcmp(curr->key, key) == 0) {
            if (prev) prev->next = curr->next;
            else table[h] = curr->next;
            free(curr);
            return 0;
        }
        prev = curr;
        curr = curr->next;
    }
    return -1;
}

// =======================
//   LIST ALL KEYS
// =======================
int kv_keys(char *out_buf, size_t out_size) {
    size_t used = 0;

    for (int i = 0; i < HASH_SIZE; i++) {
        Entry *curr = table[i];
        while (curr) {
            int written = snprintf(out_buf + used, out_size - used, "%s\n", curr->key);
            if (written < 0 || written >= (int)(out_size - used)) {
                return (int)used;
            }
            used += written;
            curr = curr->next;
        }
    }

    if (used == 0) snprintf(out_buf, out_size, "(empty)\n");
    return (int)used;
}

// =======================
//   SAVE TABLE TO DISK
// =======================
int kv_save(void) {
    FILE *fp = fopen(dump_filename, "w");
    if (!fp) return -1;

    for (int i = 0; i < HASH_SIZE; i++) {
        Entry *curr = table[i];
        while (curr) {
            fprintf(fp, "%s=%s\n", curr->key, curr->value);
            curr = curr->next;
        }
    }
    fclose(fp);
    return 0;
}
