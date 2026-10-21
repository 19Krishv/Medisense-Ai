#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CSV_FILE "Book1.csv"
static void read_value(const char *prompt, char *value, size_t size)
{
    printf("%s", prompt);
    if (fgets(value, (int)size, stdin) != NULL) {
        value[strcspn(value, "\r\n")] = '\0';
    } else {
        value[0] = '\0';
    }
}
static int read_number(const char *prompt)
{
    char input[32];
    read_value(prompt, input, sizeof(input));
    return atoi(input);
}
static void receptionist_add_patient(void)
{
    FILE *file;
    char case_id[20];
    char line[512];
    int highest_id = 0;
    int id_number;
    file = fopen(CSV_FILE, "r");
    if (file == NULL) {
        printf("Could not open the patient CSV file.\n");
        return;
    }
    while (fgets(line, sizeof(line), file) != NULL) {
        if (sscanf(line, "CASE%d,", &id_number) == 1 &&
            id_number > highest_id) {
            highest_id = id_number;
        }
    }
    fclose(file);
    snprintf(case_id, sizeof(case_id), "CASE%03d", highest_id + 1);
    printf("\n--- New Patient Intake ---\n");
    printf("Assigned case ID: %s\n", case_id);
    int age = read_number("Age: ");
    double temperature = read_number("Temperature (C): ");
    int heart_rate = read_number("Heart rate (bpm): ");
    int systolic_bp = read_number("Systolic blood pressure: ");
    int diastolic_bp = read_number("Diastolic blood pressure: ");
    int oxygen = read_number("Oxygen level (%%): ");
    int glucose = read_number("Glucose (mg/dL): ");
    printf("Enter 1 if the symptom is present, otherwise 0.\n");
    int fever = read_number("Fever: ");
    int cough = read_number("Cough: ");
    int sore_throat = read_number("Sore throat: ");
    int chest_pain = read_number("Chest pain: ");
    int headache = read_number("Headache: ");
    int vomiting = read_number("Vomiting: ");
    int abdominal_pain = read_number("Abdominal pain: ");
    int fatigue = read_number("Fatigue: ");
    int rash = read_number("Rash: ");
    int frequent_urination = read_number("Frequent urination: ");
    int blurred_vision = read_number("Blurred vision: ");
    file = fopen(CSV_FILE, "a");
    if (file == NULL) {
        printf("Could not open the patient CSV file for writing.\n");
        return;
    }
    fprintf(file,
        "%s,NO,%d,%.1f,%d,%d,%d,%d,%d,"
        "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,"
        "unclassified,pending,pending,\n",
        case_id, age, temperature, heart_rate, systolic_bp,
        diastolic_bp, oxygen, glucose, fever, cough, sore_throat,
        chest_pain, headache, vomiting, abdominal_pain, fatigue,
        rash, frequent_urination, blurred_vision);

    fclose(file);
    printf("Patient details added to Book1.csv with case ID %s.\n", case_id);
}
