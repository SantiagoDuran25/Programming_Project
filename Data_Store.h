#ifndef DATA_STORE_H
#define DATA_STORE_H

#include <stddef.h>

#define HASH_SIZE 128
#define DUMP_FILE "dump.kv"

int kv_init(const char *filename);
int kv_set(const char *key, const char *value);
int kv_get(const char *key, char *out_buf, size_t out_size);
int kv_delete(const char *key);
int kv_keys(char *out_buf, size_t out_size);
int kv_save(void);

#endif
