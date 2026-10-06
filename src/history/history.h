#ifndef HISTORY_H
#define HISTORY_H

#include <stdio.h>

typedef struct History History;

typedef struct {
    const char *visit_id;
    const char *date;
    const char *symptoms;
    const char *diagnosis;
    const char *result_or_recommendation;
    const char *doctor;
} HistoryVisit;

typedef enum {
    HISTORY_STATUS_OK,
    HISTORY_STATUS_INVALID_ARGUMENT,
    HISTORY_STATUS_DUPLICATE_VISIT,
    HISTORY_STATUS_NOT_FOUND,
    HISTORY_STATUS_OUT_OF_MEMORY,
    HISTORY_STATUS_IO_ERROR
} HistoryStatus;

/* The history stores its own copies of all supplied strings. */
HistoryStatus history_create(const char *patient_id, History **out_history);
HistoryStatus history_add_visit(History *history, const HistoryVisit *visit);
/* Lookup result strings are borrowed and remain valid until history_destroy. */
HistoryStatus history_find_visit(const History *history,
                                 const char *visit_id,
                                 HistoryVisit *out_visit);
/* Writes visits in insertion order; an empty history prints its empty-state message. */
HistoryStatus history_display(const History *history, FILE *stream);
void history_destroy(History *history);

#endif
