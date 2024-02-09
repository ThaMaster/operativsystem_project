#ifndef KVM_H
#define KVM_H

#define INSERT _IOWR('k', 'i', struct InputOutput *)
#define LOOKUP _IOWR('k', 'l', struct InputOutput *)
#define REMOVE _IOWR('k', 'r', struct InputOutput *)

struct Bucket
{
    struct KeyValuePair *value;
    struct Bucket *next;
};

struct KeyValuePair
{
    char *key;
    int key_size;
    void *value;
    int value_size;
};

int kvm_init(void);
struct KeyValuePair *kvm_lookup(const char *);
struct KeyValuePair *kvm_remove(const char *);
int kvm_insert(char* key, int key_size, void* value, int value_size);
void free_bucket(struct Bucket *);

#endif