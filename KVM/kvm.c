#include <linux/device.h>
#include <linux/err.h>
#include <linux/slab.h>

#include "kvm_ioctl.h"
#include "kvm.h"

#define NUM_BUCKETS 256

uint8_t kvm_hash(const char key[]);
int kvm_init(void);

static struct Bucket **buckets;

/**
 * Function: kvm_init
 * -------------------------
 * Initializes the key value store 
 * 
 * return: 0 on success,
 *         -1 on error, also prints error message to KERN_ERR.
 */
int kvm_init() 
{
    buckets = kcalloc(NUM_BUCKETS, sizeof(struct KeyValuePair *), GFP_KERNEL);
    if (buckets == NULL) {
        printk(KERN_ERR "ERROR: Unable to allocate memory for the bucket.");
        return -1;
    }

    return 0;
}

/**
 * Function: kvm_lookup
 * -------------------------
 * Lookup function for the hashmap. 
 *
 * key: key of key value pair to look for.
 *
 * returns: struct KeyValuePair * on success
 *          NULL otherwise
 */
struct KeyValuePair *kvm_lookup(const char *key)
{
    if (buckets == NULL) {
        printk(KERN_ERR "ERROR: Key value store has not been initialized.");
        return NULL;
    }

    // printk(KERN_INFO "kvm_lookup reached with key: \"%s\"", key);

    uint8_t hashed_key = kvm_hash(key);
    struct Bucket *bucket = buckets[hashed_key];
    if (bucket != NULL) {
        // printk(KERN_INFO "Comparing \"%s\" and \"%s\".", bucket->value->key, key);
        while (strcmp(bucket->value->key, key)) {
            if (bucket->next != NULL) {
                bucket = bucket->next;
            } else {
                printk(KERN_INFO "Unable to find KeyValuePair in bucket.");
                return NULL;
            }
            // printk(KERN_INFO "Comparing \"%s\" and \"%s\".", bucket->value->key, key);
        }
        printk(KERN_INFO "SUCCESS: Returning value: \"%s\" from lookup with key: \"%s\".", (char*)bucket->value->value, key);
        return bucket->value;
    }

    printk(KERN_INFO "Could not find bucket with key: \"%s\".\n", key);
    return NULL;
}

/**
 * Function: kvm_remove
 * -------------------------
 * Remove function for the hashmap.
 *
 * key: key of value to remove.
 *
 * returns: struct KeyValuePair * on success
 *          NULL otherwise
 */
struct KeyValuePair *kvm_remove(const char *key)
{
    if (buckets == NULL) {
        printk(KERN_ERR "ERROR: Key value store has not been initialized.");
        return NULL;
    }
    
    uint8_t hashed_key = kvm_hash(key);

    struct Bucket *prev_bucket;
    struct Bucket *curr_bucket = buckets[hashed_key];
    if(curr_bucket != NULL) {
        // Find the KeyValuePair from the key
        while(strcmp(curr_bucket->value->key, key)) {
            if(curr_bucket->next != NULL) {
                prev_bucket = curr_bucket;
                curr_bucket = curr_bucket->next;
            } else {
                printk(KERN_INFO "Unable to find the element with key: \"%s\".", key);
                return NULL;
            }
        }
        
        // Check if the desired bucket is the first in the list
        if (prev_bucket != NULL) {
            prev_bucket->next = curr_bucket->next;
        } else {
            buckets[hashed_key] = curr_bucket->next;
        }
        struct KeyValuePair *res = curr_bucket->value;
        kfree(curr_bucket);
        printk(KERN_INFO "SUCCESS: Removed bucket successfully with key: \"%s\".", key);

        return res;
    }

    printk(KERN_INFO "Unable to find the element to remove.");
    return NULL;
}

/**
 * Function: kvm_insert
 * -------------------------
 * Insert function for the hashmap. Hashes the key and inserts a key value pair
 * into the coresponding bucket.
 * 
 * kvp: Pointer to the KeyValuePair to be inserted.
 *
 * returns: 0 on success
 *          -1 otherwise
 */
int kvm_insert(char* key, int key_size, void* value, int value_size)
{
    
    if (buckets == NULL) {
        printk(KERN_ERR "ERROR: Key value store has not been initialized.");
        return -1;
    }
    printk(KERN_INFO "kvm_insert reached, key: \"%s\".", key);

    uint8_t hashed_key = kvm_hash(key);
    struct Bucket *current_bucket = buckets[hashed_key];
    struct Bucket *new_bucket;
    struct KeyValuePair *kvp = kcalloc(1, sizeof(struct KeyValuePair), GFP_KERNEL);
    
    kvp->key = key;
    kvp->key_size = key_size;
    kvp->value = value;
    kvp->value_size = value_size;

    if (current_bucket == NULL) {

        new_bucket = kcalloc(1, sizeof(struct Bucket), GFP_KERNEL);
        if (new_bucket == NULL) {
            printk(KERN_ERR "ERROR: Unable to allocate memory for bucket.\n");
            return -1;
        }
        
        new_bucket->value = kvp;
        buckets[hashed_key] = new_bucket;
    } else {
        // Loop to get to the last element in bucket list
        while (current_bucket != NULL) {
            if (strcmp(current_bucket->value->key, key)) {
                current_bucket = current_bucket->next;
            } else {
                current_bucket->value->value = value;
                printk(KERN_INFO "SUCCESS: Updated entry successfully!");
                return 0;
            }
        }

        new_bucket = kcalloc(1, sizeof(struct Bucket), GFP_KERNEL);
        if (new_bucket == NULL) {
            printk(KERN_ERR "ERROR: Unable to allocate memory for bucket.\n");
            return -1;
        }

        new_bucket->value = kvp;
        current_bucket->next = new_bucket;
    }

    printk(KERN_INFO "SUCCESS: Inserted entry successfully!");
    return 0;
}

/**
 * Function: kvm_dump
 * -------------------------
 * Dumps all the values currently stored in the
 * KVS into the Dump structure, used for persistent
 * storage outside the module.
 *
 * returns: struct Dump * on success
 *          NULL otherwise
 */
struct KeyValuePair *kvm_dump()
{
    // Go throuh all buckets
    for (int i = 0; i < NUM_BUCKETS; i++) {
        struct Bucket *bucket = buckets[i];
        
        // Find next non NULL bucket
        if (bucket != NULL) {
            return kvm_lookup(bucket->value->key);
        }
    }

    return NULL;
}

/**
 * Function: kvm_hash
 * -------------------------
 * Hashes a key of type char list into an unsigned uint8_t.
 * 
 * key: Key to be hashed of type char list.
 * 
 * returns: uint8_t.
 */
uint8_t kvm_hash(const char key[])
{
    uint8_t hash = 0;

    while (*key) {
        hash = (hash << 5) + *key++;
    }
    return hash;
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Christoffer Nordlander, Ludvig Larsson");
MODULE_DESCRIPTION("A module that provides a key-value store like functionality.\n");
MODULE_VERSION("0.3");