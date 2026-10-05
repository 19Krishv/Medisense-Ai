#ifndef SYMPTOM_HASH_H
#define SYMPTOM_HASH_H

#include <stdbool.h>
#include <stddef.h>

typedef struct SymptomHashTable SymptomHashTable;

typedef enum {
    SYMPTOM_HASH_OK,
    SYMPTOM_HASH_INVALID_ARGUMENT,
    SYMPTOM_HASH_ALREADY_EXISTS,
    SYMPTOM_HASH_NOT_FOUND,
    SYMPTOM_HASH_OUT_OF_MEMORY
} SymptomHashStatus;

SymptomHashTable *symptom_hash_create(size_t bucket_count);
SymptomHashStatus symptom_hash_insert(SymptomHashTable *table, const char *symptom);
SymptomHashStatus symptom_hash_search(const SymptomHashTable *table,
                                      const char *symptom,
                                      bool *found);
SymptomHashStatus symptom_hash_delete(SymptomHashTable *table, const char *symptom);
size_t symptom_hash_size(const SymptomHashTable *table);
void symptom_hash_destroy(SymptomHashTable *table);

#endif
