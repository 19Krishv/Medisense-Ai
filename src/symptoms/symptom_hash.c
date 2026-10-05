#include "symptom_hash.h"

#include <stdlib.h>
#include <string.h>

struct SymptomNode {
    char *text;
    struct SymptomNode *next;
};

struct SymptomHashTable {
    struct SymptomNode **buckets;
    size_t bucket_count;
    size_t size;
};

static bool is_valid_symptom(const char *symptom)
{
    return symptom != NULL && symptom[0] != '\0';
}


static size_t symptom_bucket_index(const char *symptom, size_t bucket_count)
{
    size_t hash = 5381;

    while (*symptom != '\0') {
        hash = ((hash << 5) + hash) + (unsigned char)*symptom;
        symptom++;
    }

    return hash % bucket_count;
}

SymptomHashTable *symptom_hash_create(size_t bucket_count)
{
    SymptomHashTable *table;

    if (bucket_count == 0) {
        return NULL;
    }

    table = malloc(sizeof(*table));
    if (table == NULL) {
        return NULL;
    }

    table->buckets = calloc(bucket_count, sizeof(*table->buckets));
    if (table->buckets == NULL) {
        free(table);
        return NULL;
    }

    table->bucket_count = bucket_count;
    table->size = 0;
    return table;
}

SymptomHashStatus symptom_hash_insert(SymptomHashTable *table, const char *symptom)
{
    struct SymptomNode *node;
    size_t bucket_index;
    size_t text_length;

    if (table == NULL || !is_valid_symptom(symptom)) {
        return SYMPTOM_HASH_INVALID_ARGUMENT;
    }

    bucket_index = symptom_bucket_index(symptom, table->bucket_count);
    for (node = table->buckets[bucket_index]; node != NULL; node = node->next) {
        if (strcmp(node->text, symptom) == 0) {
            return SYMPTOM_HASH_ALREADY_EXISTS;
        }
    }

    node = malloc(sizeof(*node));
    if (node == NULL) {
        return SYMPTOM_HASH_OUT_OF_MEMORY;
    }

    text_length = strlen(symptom) + 1;
    node->text = malloc(text_length);
    if (node->text == NULL) {
        free(node);
        return SYMPTOM_HASH_OUT_OF_MEMORY;
    }

    memcpy(node->text, symptom, text_length);
    node->next = table->buckets[bucket_index];
    table->buckets[bucket_index] = node;
    table->size++;
    return SYMPTOM_HASH_OK;
}

SymptomHashStatus symptom_hash_search(const SymptomHashTable *table,
                                      const char *symptom,
                                      bool *found)
{
    const struct SymptomNode *node;
    size_t bucket_index;

    if (table == NULL || !is_valid_symptom(symptom) || found == NULL) {
        return SYMPTOM_HASH_INVALID_ARGUMENT;
    }

    *found = false;
    bucket_index = symptom_bucket_index(symptom, table->bucket_count);
    for (node = table->buckets[bucket_index]; node != NULL; node = node->next) {
        if (strcmp(node->text, symptom) == 0) {
            *found = true;
            break;
        }
    }

    return SYMPTOM_HASH_OK;
}

SymptomHashStatus symptom_hash_delete(SymptomHashTable *table, const char *symptom)
{
    struct SymptomNode **link;
    size_t bucket_index;

    if (table == NULL || !is_valid_symptom(symptom)) {
        return SYMPTOM_HASH_INVALID_ARGUMENT;
    }

    bucket_index = symptom_bucket_index(symptom, table->bucket_count);
    link = &table->buckets[bucket_index];

    while (*link != NULL) {
        struct SymptomNode *node = *link;

        if (strcmp(node->text, symptom) == 0) {
            *link = node->next;
            free(node->text);
            free(node);
            table->size--;
            return SYMPTOM_HASH_OK;
        }

        link = &node->next;
    }

    return SYMPTOM_HASH_NOT_FOUND;
}

size_t symptom_hash_size(const SymptomHashTable *table)
{
    if (table == NULL) {
        return 0;
    }

    return table->size;
}

void symptom_hash_destroy(SymptomHashTable *table)
{
    size_t bucket_index;

    if (table == NULL) {
        return;
    }

    for (bucket_index = 0; bucket_index < table->bucket_count; bucket_index++) {
        struct SymptomNode *node = table->buckets[bucket_index];

        while (node != NULL) {
            struct SymptomNode *next = node->next;

            free(node->text);
            free(node);
            node = next;
        }
    }

    free(table->buckets);
    free(table);
}
