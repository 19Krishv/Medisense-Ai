#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void input_text(const char *prompt, char *value, size_t size) {
    size_t length;
    int ch;

    printf("%s", prompt);
    if (fgets(value, (int)size, stdin) == NULL) {
        value[0] = '\0';
        return;
    }

    length = strlen(value);
    if (length > 0 && value[length - 1] == '\n') {
        value[length - 1] = '\0';
    } else {
        while ((ch = getchar()) != '\n' && ch != EOF) {
        }
    }
}

static int input_choice(void) {
    char line[32];
    char *end;
    long choice;

    for (;;) {
        input_text("Select an option: ", line, sizeof(line));
        choice = strtol(line, &end, 10);
        while (*end == ' ' || *end == '\t' || *end == '\r') {
            ++end;
        }
        if (line[0] != '\0' && *end == '\0' && choice >= 0 && choice <= 3) {
            return (int)choice;
        }
        printf("Please enter 0, 1, 2, or 3.\n");
    }
}

static void doctor_input(void) {
    char doctor_name[100];
    char doctor_id[40];
    char patient_id[40];

    printf("\n--- Doctor details ---\n");
    input_text("Doctor name: ", doctor_name, sizeof(doctor_name));
    input_text("Doctor ID: ", doctor_id, sizeof(doctor_id));
    input_text("Patient ID to view: ", patient_id, sizeof(patient_id));

}

static void patient_input(void) {
    char patient_id[40];
    char patient_name[100];
    char reason[200];
    char appointment_choice[16];

    printf("\n--- Patient details ---\n");
    input_text("Patient ID: ", patient_id, sizeof(patient_id));
    input_text("Patient name: ", patient_name, sizeof(patient_name));
    input_text("Reason for visit or report enquiry: ", reason, sizeof(reason));
    input_text("Would you like to request an appointment? (yes/no): ",
               appointment_choice, sizeof(appointment_choice));

}

static void receptionist_input(void) {
    char patient_name[100];
    char age[16];
    char sex[32];
    char phone[40];
    char symptoms[200];
    char vitals[160];

    printf("\n--- Receptionist patient intake ---\n");
    input_text("Patient name: ", patient_name, sizeof(patient_name));
    input_text("Age: ", age, sizeof(age));
    input_text("Sex/gender: ", sex, sizeof(sex));
    input_text("Phone number: ", phone, sizeof(phone));
    input_text("Symptoms / reason for visit: ", symptoms, sizeof(symptoms));
    input_text("Vitals (optional): ", vitals, sizeof(vitals));

}

int main(void) {
    int choice;

    for (;;) {
        printf(" MediSense AI - Details Input Menu\n");
        printf("\n");
        printf("1. Doctor\n");
        printf("2. Patient\n");
        printf("3. Receptionist\n");
        printf("0. Exit\n");

        choice = input_choice();
        if (choice == 0) {
            break;
        } else if (choice == 1) {
            doctor_input();
        } else if (choice == 2) {
            patient_input();
        } else {
            receptionist_input();
        }
    }

    return 0;
}
