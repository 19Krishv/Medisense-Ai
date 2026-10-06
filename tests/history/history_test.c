#include "../../src/history/history.h"

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INPUT_CAPACITY 256

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

static int read_nonempty_line(const char *prompt, char *buffer, size_t capacity)
{
    for (;;) {
        size_t length;
        int character;

        printf("%s", prompt);
        if (fgets(buffer, (int)capacity, stdin) == NULL) {
            return 0;
        }

        length = strlen(buffer);
        if (length > 0 && buffer[length - 1] == '\n') {
            buffer[length - 1] = '\0';
        } else if (!feof(stdin)) {
            while ((character = getchar()) != '\n' && character != EOF) {
                /* Discard the remainder of an overlong input line. */
            }
            puts("Input is too long; please try again.");
            continue;
        }

        if (buffer[0] == '\0') {
            puts("This value cannot be empty; please try again.");
            continue;
        }

        return 1;
    }
}

static int read_visit_count(size_t *out_count)
{
    char input[INPUT_CAPACITY];
    char *end;
    unsigned long long parsed_count;

    for (;;) {
        printf("Number of visits (0 or more): ");
        if (fgets(input, sizeof(input), stdin) == NULL) {
            return 0;
        }

        if (strchr(input, '\n') == NULL && !feof(stdin)) {
            int character;

            while ((character = getchar()) != '\n' && character != EOF) {
                /* Discard the remainder of an overlong input line. */
            }
            puts("Input is too long; please enter a visit count.");
            continue;
        }

        end = input;
        while (isspace((unsigned char)*end)) {
            end++;
        }
        if (*end == '-') {
            puts("Please enter a valid non-negative whole number.");
            continue;
        }

        errno = 0;
        parsed_count = strtoull(input, &end, 10);
        while (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r') {
            end++;
        }

        if (input == end || *end != '\0' || errno == ERANGE
            || parsed_count > SIZE_MAX) {
            puts("Please enter a valid non-negative whole number.");
            continue;
        }

        *out_count = (size_t)parsed_count;
        return 1;
    }
}

static int run_interactive_history(void)
{
    char patient_id[INPUT_CAPACITY];
    History *history = NULL;
    HistoryStatus status;
    size_t visit_count;
    size_t index;

    puts("Patient visit history");
    if (!read_nonempty_line("Patient ID: ", patient_id, sizeof(patient_id))
        || !read_visit_count(&visit_count)) {
        fputs("\nInput ended before history entry was complete.\n", stderr);
        return 1;
    }

    status = history_create(patient_id, &history);
    if (status != HISTORY_STATUS_OK) {
        fprintf(stderr, "Could not create history (status %d).\n", status);
        return 1;
    }

    for (index = 0; index < visit_count; index++) {
        char visit_id[INPUT_CAPACITY];
        char date[INPUT_CAPACITY];
        char symptoms[INPUT_CAPACITY];
        char diagnosis[INPUT_CAPACITY];
        char result_or_recommendation[INPUT_CAPACITY];
        char doctor[INPUT_CAPACITY];
        HistoryVisit visit;

        printf("\nVisit %zu of %zu\n", index + 1, visit_count);
        if (!read_nonempty_line("Visit ID: ", visit_id, sizeof(visit_id))
            || !read_nonempty_line("Date: ", date, sizeof(date))
            || !read_nonempty_line("Symptoms: ", symptoms, sizeof(symptoms))
            || !read_nonempty_line("Diagnosis: ", diagnosis, sizeof(diagnosis))
            || !read_nonempty_line("Result/recommendation: ",
                                   result_or_recommendation,
                                   sizeof(result_or_recommendation))
            || !read_nonempty_line("Doctor: ", doctor, sizeof(doctor))) {
            fputs("\nInput ended before the visit was complete.\n", stderr);
            history_destroy(history);
            return 1;
        }

        visit = make_visit(visit_id,
                           date,
                           symptoms,
                           diagnosis,
                           result_or_recommendation,
                           doctor);
        status = history_add_visit(history, &visit);
        if (status != HISTORY_STATUS_OK) {
            fprintf(stderr, "Could not add visit (status %d).\n", status);
            history_destroy(history);
            return 1;
        }
    }

    puts("\nStored visit history:");
    status = history_display(history, stdout);
    history_destroy(history);

    if (status != HISTORY_STATUS_OK) {
        fprintf(stderr, "Could not display history (status %d).\n", status);
        return 1;
    }

    return 0;
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

    puts("Sample history output:");
    assert(history_display(history, stdout) == HISTORY_STATUS_OK);

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

int main(int argc, char **argv)
{
    if (argc == 2 && strcmp(argv[1], "--interactive") == 0) {
        return run_interactive_history();
    }

    if (argc != 1) {
        fprintf(stderr, "Usage: %s [--interactive]\n", argv[0]);
        return 2;
    }

    test_create_and_empty_history();
    test_add_one_visit_and_copy_input();
    test_multiple_visits_are_displayed_in_order();
    test_missing_and_duplicate_visit_ids();
    test_histories_are_isolated_by_patient();
    test_invalid_arguments_and_cleanup();
    puts("All history tests passed.");
    return 0;
}
