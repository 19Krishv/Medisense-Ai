#include "symptom_hash.h"

#include <assert.h>

static void test_creation_and_empty_table(void)
{
    SymptomHashTable *table = symptom_hash_create(8);
    bool found = true;

    assert(table != NULL);
    assert(symptom_hash_size(table) == 0);
    assert(symptom_hash_search(table, "fever", &found) == SYMPTOM_HASH_OK);
    assert(!found);
    assert(symptom_hash_delete(table, "fever") == SYMPTOM_HASH_NOT_FOUND);

    symptom_hash_destroy(table);
    assert(symptom_hash_create(0) == NULL);
}

static void test_insert_and_search(void)
{
    SymptomHashTable *table = symptom_hash_create(8);
    char symptom[] = "headache";
    bool found = false;

    assert(table != NULL);
    assert(symptom_hash_insert(table, symptom) == SYMPTOM_HASH_OK);
    symptom[0] = 'X';

    assert(symptom_hash_search(table, "headache", &found) == SYMPTOM_HASH_OK);
    assert(found);
    assert(symptom_hash_search(table, "nausea", &found) == SYMPTOM_HASH_OK);
    assert(!found);
    assert(symptom_hash_insert(table, "fever") == SYMPTOM_HASH_OK);
    assert(symptom_hash_size(table) == 2);

    symptom_hash_destroy(table);
}

static void test_collisions_and_duplicates(void)
{
    SymptomHashTable *table = symptom_hash_create(1);
    const char *symptoms[] = {"fever", "rash", "cough"};
    size_t index;
    bool found;

    assert(table != NULL);
    for (index = 0; index < sizeof(symptoms) / sizeof(symptoms[0]); index++) {
        assert(symptom_hash_insert(table, symptoms[index]) == SYMPTOM_HASH_OK);
    }

    for (index = 0; index < sizeof(symptoms) / sizeof(symptoms[0]); index++) {
        assert(symptom_hash_search(table, symptoms[index], &found) == SYMPTOM_HASH_OK);
        assert(found);
    }

    assert(symptom_hash_insert(table, "fever") == SYMPTOM_HASH_ALREADY_EXISTS);
    assert(symptom_hash_size(table) == 3);
    symptom_hash_destroy(table);
}

static void test_delete_from_chain(void)
{
    SymptomHashTable *table = symptom_hash_create(1);
    bool found;

    assert(table != NULL);
    assert(symptom_hash_insert(table, "fever") == SYMPTOM_HASH_OK);
    assert(symptom_hash_insert(table, "rash") == SYMPTOM_HASH_OK);
    assert(symptom_hash_insert(table, "cough") == SYMPTOM_HASH_OK);

    assert(symptom_hash_delete(table, "rash") == SYMPTOM_HASH_OK);
    assert(symptom_hash_search(table, "rash", &found) == SYMPTOM_HASH_OK);
    assert(!found);
    assert(symptom_hash_size(table) == 2);
    assert(symptom_hash_delete(table, "cough") == SYMPTOM_HASH_OK);
    assert(symptom_hash_delete(table, "fever") == SYMPTOM_HASH_OK);
    assert(symptom_hash_delete(table, "fever") == SYMPTOM_HASH_NOT_FOUND);
    assert(symptom_hash_size(table) == 0);

    symptom_hash_destroy(table);
}

static void test_invalid_inputs_and_null_cleanup(void)
{
    SymptomHashTable *table = symptom_hash_create(4);
    bool found;

    assert(table != NULL);
    assert(symptom_hash_insert(NULL, "fever") == SYMPTOM_HASH_INVALID_ARGUMENT);
    assert(symptom_hash_insert(table, NULL) == SYMPTOM_HASH_INVALID_ARGUMENT);
    assert(symptom_hash_insert(table, "") == SYMPTOM_HASH_INVALID_ARGUMENT);
    assert(symptom_hash_search(NULL, "fever", &found) == SYMPTOM_HASH_INVALID_ARGUMENT);
    assert(symptom_hash_search(table, NULL, &found) == SYMPTOM_HASH_INVALID_ARGUMENT);
    assert(symptom_hash_search(table, "fever", NULL) == SYMPTOM_HASH_INVALID_ARGUMENT);
    assert(symptom_hash_delete(NULL, "fever") == SYMPTOM_HASH_INVALID_ARGUMENT);
    assert(symptom_hash_delete(table, NULL) == SYMPTOM_HASH_INVALID_ARGUMENT);
    assert(symptom_hash_size(NULL) == 0);

    symptom_hash_destroy(NULL);
    symptom_hash_destroy(table);
}

int main(void)
{
    test_creation_and_empty_table();
    test_insert_and_search();
    test_collisions_and_duplicates();
    test_delete_from_chain();
    test_invalid_inputs_and_null_cleanup();
    return 0;
}
