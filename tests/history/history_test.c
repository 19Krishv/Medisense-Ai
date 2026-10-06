#include "../../src/history/history.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static HistoryVisit make_visit(const char *visit_id,
                               const char *date,
                               const char *symptoms,
                               const char *diagnosis,
                               const char *result_or_recommendation,
                               const char *doctor)
{
    HistoryVisit visit = {
        visit_id,
        date,
        symptoms,
        diagnosis,
        result_or_recommendation,
        doctor
    };

    return visit;
}

static size_t read_stream(FILE *stream, char *buffer, size_t capacity)
{
    size_t bytes_read;

    assert(stream != NULL);
    assert(buffer != NULL);
    assert(capacity > 0);
    assert(fseek(stream, 0, SEEK_SET) == 0);

    bytes_read = fread(buffer, 1, capacity - 1, stream);
    assert(!ferror(stream));
    buffer[bytes_read] = '\0';
    return bytes_read;
}

static void test_create_and_empty_history(void)
{
    History *history = NULL;
    HistoryVisit result = {0};
    FILE *stream;
    char output[128];

    assert(history_create("patient-1", &history) == HISTORY_STATUS_OK);
    assert(history != NULL);
    assert(history_find_visit(history, "visit-1", &result) == HISTORY_STATUS_NOT_FOUND);
    assert(result.visit_id == NULL);

    stream = tmpfile();
    assert(stream != NULL);
    assert(history_display(history, stream) == HISTORY_STATUS_OK);
    read_stream(stream, output, sizeof(output));
    assert(strcmp(output, "No visit history available.\n") == 0);

    fclose(stream);
    history_destroy(history);
}

static void test_add_one_visit_and_copy_input(void)
{
    History *history = NULL;
    HistoryVisit result = {0};
    char visit_id[] = "visit-1";
    char date[] = "2026-10-01";
    HistoryVisit visit = make_visit(visit_id,
                                    date,
                                    "fever",
                                    "viral infection",
                                    "rest and fluids",
                                    "Dr. Rao");

    assert(history_create("patient-1", &history) == HISTORY_STATUS_OK);
    assert(history_add_visit(history, &visit) == HISTORY_STATUS_OK);

    visit_id[0] = 'X';
    date[0] = 'X';
    assert(history_find_visit(history, "visit-1", &result) == HISTORY_STATUS_OK);
    assert(strcmp(result.visit_id, "visit-1") == 0);
    assert(strcmp(result.date, "2026-10-01") == 0);
    assert(strcmp(result.symptoms, "fever") == 0);
    assert(strcmp(result.diagnosis, "viral infection") == 0);
    assert(strcmp(result.result_or_recommendation, "rest and fluids") == 0);
    assert(strcmp(result.doctor, "Dr. Rao") == 0);

    history_destroy(history);
}

static void test_multiple_visits_are_displayed_in_order(void)
{
    History *history = NULL;
    HistoryVisit result = {0};
    HistoryVisit first = make_visit("visit-1", "2026-10-01", "fever",
                                    "viral infection", "rest", "Dr. Rao");
    HistoryVisit second = make_visit("visit-2", "2026-10-10", "cough",
                                     "bronchitis", "follow-up", "Dr. Kim");
    HistoryVisit third = make_visit("visit-3", "2026-10-20", "headache",
                                    "migraine", "medication", "Dr. Lee");
    FILE *stream;
    char output[1024];
    char *first_position;
    char *second_position;
    char *third_position;

    assert(history_create("patient-1", &history) == HISTORY_STATUS_OK);
    assert(history_add_visit(history, &first) == HISTORY_STATUS_OK);
    assert(history_add_visit(history, &second) == HISTORY_STATUS_OK);
    assert(history_add_visit(history, &third) == HISTORY_STATUS_OK);

    assert(history_find_visit(history, "visit-1", &result) == HISTORY_STATUS_OK);
    assert(strcmp(result.symptoms, "fever") == 0);
    assert(history_find_visit(history, "visit-2", &result) == HISTORY_STATUS_OK);
    assert(strcmp(result.diagnosis, "bronchitis") == 0);
    assert(history_find_visit(history, "visit-3", &result) == HISTORY_STATUS_OK);
    assert(strcmp(result.doctor, "Dr. Lee") == 0);

    stream = tmpfile();
    assert(stream != NULL);
    assert(history_display(history, stream) == HISTORY_STATUS_OK);
    read_stream(stream, output, sizeof(output));

    first_position = strstr(output, "Visit ID: visit-1");
    second_position = strstr(output, "Visit ID: visit-2");
    third_position = strstr(output, "Visit ID: visit-3");
    assert(first_position != NULL);
    assert(second_position != NULL);
    assert(third_position != NULL);
    assert(first_position < second_position);
    assert(second_position < third_position);
    assert(strstr(output, "Patient ID: patient-1") != NULL);
    assert(strstr(output, "Symptoms: headache") != NULL);

    fclose(stream);
    history_destroy(history);
}

static void test_missing_and_duplicate_visit_ids(void)
{
    History *history = NULL;
    HistoryVisit result = {0};
    HistoryVisit visit = make_visit("visit-1", "2026-10-01", "fever",
                                    "viral infection", "rest", "Dr. Rao");

    assert(history_create("patient-1", &history) == HISTORY_STATUS_OK);
    assert(history_add_visit(history, &visit) == HISTORY_STATUS_OK);
    assert(history_add_visit(history, &visit) == HISTORY_STATUS_DUPLICATE_VISIT);
    assert(history_find_visit(history, "missing-visit", &result) == HISTORY_STATUS_NOT_FOUND);
    assert(result.visit_id == NULL);

    history_destroy(history);
}

static void test_histories_are_isolated_by_patient(void)
{
    History *first_history = NULL;
    History *second_history = NULL;
    HistoryVisit first_result = {0};
    HistoryVisit second_result = {0};
    HistoryVisit first_visit = make_visit("visit-1", "2026-10-01", "fever",
                                          "viral infection", "rest", "Dr. Rao");
    HistoryVisit second_visit = make_visit("visit-1", "2026-10-02", "cough",
                                           "bronchitis", "follow-up", "Dr. Kim");

    assert(history_create("patient-1", &first_history) == HISTORY_STATUS_OK);
    assert(history_create("patient-2", &second_history) == HISTORY_STATUS_OK);
    assert(history_add_visit(first_history, &first_visit) == HISTORY_STATUS_OK);
    assert(history_add_visit(second_history, &second_visit) == HISTORY_STATUS_OK);

    assert(history_find_visit(first_history, "visit-1", &first_result) == HISTORY_STATUS_OK);
    assert(history_find_visit(second_history, "visit-1", &second_result) == HISTORY_STATUS_OK);
    assert(strcmp(first_result.symptoms, "fever") == 0);
    assert(strcmp(second_result.symptoms, "cough") == 0);

    history_destroy(first_history);
    history_destroy(second_history);
}

static void test_invalid_arguments_and_cleanup(void)
{
    History *history = NULL;
    HistoryVisit visit = make_visit("visit-1", "2026-10-01", "fever",
                                    "viral infection", "rest", "Dr. Rao");
    HistoryVisit invalid_visit = make_visit("visit-2", "2026-10-02", "",
                                            "diagnosis", "result", "doctor");
    HistoryVisit result = {0};

    assert(history_create(NULL, &history) == HISTORY_STATUS_INVALID_ARGUMENT);
    assert(history == NULL);
    assert(history_create("", &history) == HISTORY_STATUS_INVALID_ARGUMENT);
    assert(history_create("patient-1", NULL) == HISTORY_STATUS_INVALID_ARGUMENT);
    assert(history_create("patient-1", &history) == HISTORY_STATUS_OK);

    assert(history_add_visit(NULL, &visit) == HISTORY_STATUS_INVALID_ARGUMENT);
    assert(history_add_visit(history, NULL) == HISTORY_STATUS_INVALID_ARGUMENT);
    assert(history_add_visit(history, &invalid_visit) == HISTORY_STATUS_INVALID_ARGUMENT);
    assert(history_find_visit(NULL, "visit-1", &result) == HISTORY_STATUS_INVALID_ARGUMENT);
    assert(history_find_visit(history, "", &result) == HISTORY_STATUS_INVALID_ARGUMENT);
    assert(history_find_visit(history, "visit-1", NULL) == HISTORY_STATUS_INVALID_ARGUMENT);
    assert(history_display(NULL, stdout) == HISTORY_STATUS_INVALID_ARGUMENT);
    assert(history_display(history, NULL) == HISTORY_STATUS_INVALID_ARGUMENT);

    history_destroy(NULL);
    history_destroy(history);
}

int main(void)
{
    test_create_and_empty_history();
    test_add_one_visit_and_copy_input();
    test_multiple_visits_are_displayed_in_order();
    test_missing_and_duplicate_visit_ids();
    test_histories_are_isolated_by_patient();
    test_invalid_arguments_and_cleanup();
    return 0;
}
