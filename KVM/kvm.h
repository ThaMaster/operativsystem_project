#ifndef KVM_H
#define KVM_H

#define INSERT _IOWR('k', 'i', struct InputOutput *)
#define LOOKUP _IOWR('k', 'l', struct InputOutput *)
#define REMOVE _IOWR('k', 'r', struct InputOutput *)

uint8_t kvm_hash(const char key[]);
int kvm_init(void);
struct KeyValuePair *kvm_lookup(const char[]);
int kvm_remove(const char[]);
int kvm_insert(struct KeyValuePair *);

struct Bucket
{
    struct KeyValuePair *value;
    struct Bucket *next;
};

struct KeyValuePair
{
    char key[32];
    void *value;
    int size;
};

#endif