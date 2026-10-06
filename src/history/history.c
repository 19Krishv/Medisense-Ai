#include "history.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

struct HistoryVisitNode {
    char *visit_id;
    char *date;
    char *symptoms;
    char *diagnosis;
    char *result_or_recommendation;
    char *doctor;
    struct HistoryVisitNode *next;
};

struct History {
    char *patient_id;
    struct HistoryVisitNode *head;
    struct HistoryVisitNode *tail;
};

static bool is_nonempty_string(const char *text)
{
    return text != NULL && text[0] != '\0';
}

static char *copy_string(const char *text)
{
    size_t length = strlen(text) + 1;
    char *copy = malloc(length);

    if (copy != NULL) {
        memcpy(copy, text, length);
    }

    return copy;
}

static void free_visit_node(struct HistoryVisitNode *node)
{
    if (node == NULL) {
        return;
    }

    free(node->visit_id);
    free(node->date);
    free(node->symptoms);
    free(node->diagnosis);
    free(node->result_or_recommendation);
    free(node->doctor);
    free(node);
}

HistoryStatus history_create(const char *patient_id, History **out_history)
{
    History *history;

    if (out_history == NULL) {
        return HISTORY_STATUS_INVALID_ARGUMENT;
    }

    *out_history = NULL;
    if (!is_nonempty_string(patient_id)) {
        return HISTORY_STATUS_INVALID_ARGUMENT;
    }

    history = malloc(sizeof(*history));
    if (history == NULL) {
        return HISTORY_STATUS_OUT_OF_MEMORY;
    }

    history->patient_id = copy_string(patient_id);
    if (history->patient_id == NULL) {
        free(history);
        return HISTORY_STATUS_OUT_OF_MEMORY;
    }

    history->head = NULL;
    history->tail = NULL;
    *out_history = history;
    return HISTORY_STATUS_OK;
}

static bool is_valid_visit(const HistoryVisit *visit)
{
    return visit != NULL
        && is_nonempty_string(visit->visit_id)
        && is_nonempty_string(visit->date)
        && is_nonempty_string(visit->symptoms)
        && is_nonempty_string(visit->diagnosis)
        && is_nonempty_string(visit->result_or_recommendation)
        && is_nonempty_string(visit->doctor);
}

static bool visit_id_exists(const History *history, const char *visit_id)
{
    const struct HistoryVisitNode *node;

    for (node = history->head; node != NULL; node = node->next) {
        if (strcmp(node->visit_id, visit_id) == 0) {
            return true;
        }
    }

    return false;
}

HistoryStatus history_add_visit(History *history, const HistoryVisit *visit)
{
    struct HistoryVisitNode *node;

    if (history == NULL || !is_valid_visit(visit)) {
        return HISTORY_STATUS_INVALID_ARGUMENT;
    }

    if (visit_id_exists(history, visit->visit_id)) {
        return HISTORY_STATUS_DUPLICATE_VISIT;
    }

    node = calloc(1, sizeof(*node));
    if (node == NULL) {
        return HISTORY_STATUS_OUT_OF_MEMORY;
    }

    node->visit_id = copy_string(visit->visit_id);
    node->date = copy_string(visit->date);
    node->symptoms = copy_string(visit->symptoms);
    node->diagnosis = copy_string(visit->diagnosis);
    node->result_or_recommendation = copy_string(visit->result_or_recommendation);
    node->doctor = copy_string(visit->doctor);

    if (node->visit_id == NULL
        || node->date == NULL
        || node->symptoms == NULL
        || node->diagnosis == NULL
        || node->result_or_recommendation == NULL
        || node->doctor == NULL) {
        free_visit_node(node);
        return HISTORY_STATUS_OUT_OF_MEMORY;
    }

    if (history->tail == NULL) {
        history->head = node;
    } else {
        history->tail->next = node;
    }

    history->tail = node;
    return HISTORY_STATUS_OK;
}

HistoryStatus history_find_visit(const History *history,
                                 const char *visit_id,
                                 HistoryVisit *out_visit)
{
    const struct HistoryVisitNode *node;

    if (out_visit == NULL) {
        return HISTORY_STATUS_INVALID_ARGUMENT;
    }

    *out_visit = (HistoryVisit){0};
    if (history == NULL || !is_nonempty_string(visit_id)) {
        return HISTORY_STATUS_INVALID_ARGUMENT;
    }

    for (node = history->head; node != NULL; node = node->next) {
        if (strcmp(node->visit_id, visit_id) == 0) {
            out_visit->visit_id = node->visit_id;
            out_visit->date = node->date;
            out_visit->symptoms = node->symptoms;
            out_visit->diagnosis = node->diagnosis;
            out_visit->result_or_recommendation = node->result_or_recommendation;
            out_visit->doctor = node->doctor;
            return HISTORY_STATUS_OK;
        }
    }

    return HISTORY_STATUS_NOT_FOUND;
}

HistoryStatus history_display(const History *history, FILE *stream)
{
    const struct HistoryVisitNode *node;

    if (history == NULL || stream == NULL) {
        return HISTORY_STATUS_INVALID_ARGUMENT;
    }

    if (history->head == NULL) {
        return fprintf(stream, "No visit history available.\n") < 0
            ? HISTORY_STATUS_IO_ERROR
            : HISTORY_STATUS_OK;
    }

    for (node = history->head; node != NULL; node = node->next) {
        if (fprintf(stream,
                    "Patient ID: %s\n"
                    "Visit ID: %s\n"
                    "Date: %s\n"
                    "Symptoms: %s\n"
                    "Diagnosis: %s\n"
                    "Result/recommendation: %s\n"
                    "Doctor: %s\n",
                    history->patient_id,
                    node->visit_id,
                    node->date,
                    node->symptoms,
                    node->diagnosis,
                    node->result_or_recommendation,
                    node->doctor) < 0) {
            return HISTORY_STATUS_IO_ERROR;
        }
    }

    return HISTORY_STATUS_OK;
}

void history_destroy(History *history)
{
    struct HistoryVisitNode *node;

    if (history == NULL) {
        return;
    }

    node = history->head;
    while (node != NULL) {
        struct HistoryVisitNode *next = node->next;

        free_visit_node(node);
        node = next;
    }

    free(history->patient_id);
    free(history);
}
