#ifndef HASH_H_INCLUDED
#define HASH_H_INCLUDED

#include "common.h"

typedef struct hash hash_t;
typedef unsigned (*hashfunc_t)(unsigned,void*);

hash_t* hash_alloc(unsigned buckets,hashfunc_t hash_func);
void* hash_lookup_entry(hash_t* hash,void* key,unsigned key_size);
void hash_add_entry(hash_t* hash,void* key,unsigned key_size,void* value,unsigned value_size);
void hash_free_entry(hash_t* hash,void* key,unsigned key_size);


#endif // HASH_H_INCLUDED
