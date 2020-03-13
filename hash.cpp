#include"hash.h"
#include"common.h"

typedef struct hash_node {
    void *key;
    void *value;
    struct hash_node* prev;
    struct hash_node* next;
} hash_node_t;

///别名hash_t
typedef struct hash {
    unsigned buckets;
    hashfunc_t hash_func;
    hash_node_t **nodes;///确实是**node,挂链,数组每个元素都是一个指针
} hash_t;

hash_node_t* hash_get_node_by_key(hash_t* h,void *key,unsigned size);
hash_node_t** hash_get_bucket(hash_t *h,void* key);


hash_t* hash_alloc(unsigned buckets,hashfunc_t func) {
    hash_t* h=(hash_t*)malloc(sizeof(hash_t));
    h->buckets=buckets;
    h->hash_func=func;
    int sz=buckets*sizeof(hash_node_t*);
    h->nodes=(hash_node_t**)malloc(sz);
    bzero(h->nodes,sz);
    return h;
}

void* hash_lookup_entry(hash_t *h,void* key,unsigned size) {
    hash_node_t* pnode=hash_get_node_by_key(h,key,size);
    return pnode?pnode->value:NULL;
}

void hash_add_entry(hash_t* h,void* key,unsigned ksize,void* value,unsigned vsize) {
    if(hash_lookup_entry(h,key,ksize)!=NULL) {
        fprintf(stderr,"duplicate key.\n");
        return;
    }
    hash_node_t* node=(hash_node_t*)malloc(sizeof(hash_node_t));
    node->prev=node->next=NULL;

    node->key=malloc(ksize);
    memcpy(node->key,key,ksize);

    node->value=malloc(vsize);
    memcpy(node->value,value,vsize);

    hash_node_t **bucket=hash_get_bucket(h,key);
    if(*bucket==NULL)*bucket=node;
    else {
        node->next=*bucket;
        (*bucket)->prev=node;
        *bucket=node;
    }
}

void hash_free_entry(hash_t* h,void* key,unsigned ksize) {
    hash_node_t* node=hash_get_node_by_key(h,key,ksize);
    if(node==NULL) {
        fprintf(stderr,"no such key.\n");
        return;
    }
    free(node->key);
    free(node->value);

    if(node->prev)node->prev->next=node->next;
    else {
        hash_node_t** bucket=hash_get_bucket(h,key);
        *bucket=node->next;
    }
    if(node->next)node->next->prev=node->prev;

    free(node);
}

///get数组中某个元素的指针
hash_node_t** hash_get_bucket(hash_t *h,void* key) {
    unsigned id=h->hash_func(h->buckets,key);
    if(id>=h->buckets) {
        ERR_EXIT("hash_get_bucket");
    }
    return &(h->nodes[id]);
}

hash_node_t* hash_get_node_by_key(hash_t* h,void *key,unsigned size) {
    hash_node_t** bucket=hash_get_bucket(h,key);
    if(bucket==NULL){
        ERR_EXIT("hash_get_node_by_key->hash_get_bucket");
    }
    for(hash_node_t* ret=*bucket; ret; ret=ret->next) {
        if(memcmp(ret->key,key,size)==0)
            return ret;
    }
    return NULL;
}







