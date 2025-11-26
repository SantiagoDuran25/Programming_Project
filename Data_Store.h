// kvstore.h
#ifndef KVSTORE_H
#define KVSTORE_H

#include <stddef.h>

void kv_init(const char *filename);   // load from file if exists
void kv_shutdown(void);               // save + free

int  kv_set(const char *key, const char *value);
int  kv_get(const char *key, char *out_value, size_t out_size);
int  kv_del(const char *key);
int  kv_keys(char *out_buf, size_t out_size);  // returns number of keys
int  kv_save(void);                            // force save to file

#endif

