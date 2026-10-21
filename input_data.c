#include "medisense.h"

const char *symptom_labels[SYMPTOM_COUNT] = {
    "Fever", "Cough", "Sore throat", "Chest pain", "Headache", "Vomiting",
    "Abdominal pain", "Fatigue", "Rash", "Frequent urination", "Blurred vision"
};

const char *patient_csv_header =
    "caseId,synthetic,age,temperature_c,heart_rate_bpm,systolic_bp_mmhg,"
    "diastolic_bp_mmhg,oxygen_level_percent,glucose_mg_dl,fever,cough,sore_throat,"
    "chest_pain,headache,vomiting,abdominal_pain,fatigue,rash,frequent_urination,"
    "blurred_vision,caseCategory,urgencyLabel,recommendedSpecialist,recordType,"
    "patientId,pin,patientName,sex,phone,reportDate,reportTitle,reportNotes,"
    "appointmentDate,appointmentTime,appointmentDoctor,appointmentStatus,"
    "doctorRecommendedCategory,appointmentReason\n";


void read_user_input(const char *prompt, char *out, size_t capacity) {
    size_t n; int ch;
    printf("%s", prompt);
    if (!fgets(out, (int)capacity, stdin)) { out[0] = '\0'; return; }
    n = strlen(out);
    if (n && out[n - 1] == '\n') out[n - 1] = '\0';
    else while ((ch = getchar()) != '\n' && ch != EOF) { }
    for (n = 0; out[n]; ++n)
        if (out[n] == ',' || out[n] == '\r' || out[n] == '\n') out[n] = ' ';
}

int read_validated_integer(const char *prompt, int low, int high) {
    char s[64], *end; long v;
    for (;;) {
        read_user_input(prompt, s, sizeof(s)); v = strtol(s, &end, 10);
        while (isspace((unsigned char)*end)) ++end;
        if (s[0] && *end == '\0' && v >= low && v <= high) return (int)v;
        printf("Enter a number from %d to %d.\n", low, high);
    }
}

double read_validated_decimal(const char *prompt, double low, double high) {
    char s[64], *end; double v;
    for (;;) {
        read_user_input(prompt, s, sizeof(s)); v = strtod(s, &end);
        while (isspace((unsigned char)*end)) ++end;
        if (s[0] && *end == '\0' && v >= low && v <= high) return v;
        printf("Enter a number from %.1f to %.1f.\n", low, high);
    }
}

void wait_for_enter(void) { char s[8]; read_user_input("\nPress Enter to continue...", s, sizeof(s)); }

int split_csv_row(char *line, char *fields[], int max_fields) {
    int item_count = 0;
    size_t len = strlen(line);
    while (len && (line[len - 1] == '\n' || line[len - 1] == '\r')) line[--len] = '\0';
    if (!len) return 0;
    fields[item_count++] = line;
    for (char *p = line; *p && item_count < max_fields; ++p) {
        if (*p == ',') { *p = '\0'; fields[item_count++] = p + 1; }
    }
    return item_count;
}

void create_patient_file_if_needed(void) {
    FILE *f = fopen(PATIENT_FILE, "a+");
    if (!f) { printf("Cannot open %s.\n", PATIENT_FILE); return; }
    fseek(f, 0, SEEK_END);
    if (ftell(f) == 0) { fputs(patient_csv_header, f); }
    fclose(f);
}

void load_case_from_csv_fields(char *f[], Case *c) {
    memset(c, 0, sizeof(*c));
    snprintf(c->case_id, sizeof(c->case_id), "%s", f[0]);
    snprintf(c->is_synthetic, sizeof(c->is_synthetic), "%s", f[1]);
    c->age = atoi(f[2]); c->body_temperature = strtod(f[3], NULL);
    for (int i = 0; i < VITAL_COUNT; ++i) c->vital_signs[i] = strtod(f[4 + i], NULL);
    for (int i = 0; i < SYMPTOM_COUNT; ++i) c->symptom_present[i] = atoi(f[9 + i]) != 0;
    snprintf(c->case_category, sizeof(c->case_category), "%s", f[20]);
    snprintf(c->urgency_label, sizeof(c->urgency_label), "%s", f[21]);
    snprintf(c->recommended_specialty, sizeof(c->recommended_specialty), "%s", f[22]);
}

int load_labeled_cases(Case cases[], int max_cases) {
    FILE *file = fopen(BOOK_FILE, "r"); char line[CSV_LINE_CAPACITY]; int item_count = 0;
    if (!file) { printf("Cannot open %s. Put it beside the program.\n", BOOK_FILE); return -1; }
    (void)fgets(line, sizeof(line), file);
    while (item_count < max_cases && fgets(line, sizeof(line), file)) {
        char *f[CSV_FIELD_CAPACITY] = {0};
        if (split_csv_row(line, f, CSV_FIELD_CAPACITY) < 23 || !f[0][0] || !f[20][0] ||
            strcmp(f[20], "unclassified") == 0 || strcmp(f[20], "pending") == 0) continue;
        load_case_from_csv_fields(f, &cases[item_count++]);
    }
    fclose(file); return item_count;
}

void create_next_case_id(const char *path, const char *prefix, char *out, size_t size) {
    FILE *file = fopen(path, "r"); char line[CSV_LINE_CAPACITY], key[40]; int max = 0, value;
    if (file) {
        while (fgets(line, sizeof(line), file)) {
            if (sscanf(line, "%39[^,],", key) == 1 &&
                sscanf(key, "CASE%d", &value) == 1 && value > max) max = value;
        }
        fclose(file);
    }
    snprintf(out, size, "%s%03d", prefix, max + 1);
}

void create_next_patient_id(char *out, size_t size) {
    FILE *file = fopen(PATIENT_FILE, "r"); char line[CSV_LINE_CAPACITY]; int max = 1000;
    if (file) {
        (void)fgets(line, sizeof(line), file);
        while (fgets(line, sizeof(line), file)) {
            char *f[CSV_FIELD_CAPACITY] = {0};
            if (split_csv_row(line, f, CSV_FIELD_CAPACITY) > 24 && strcmp(f[23], "PROFILE") == 0 &&
                strncmp(f[24], "P", 1) == 0) {
                int value = atoi(f[24] + 1); if (value > max) max = value;
            }
        }
        fclose(file);
    }
    snprintf(out, size, "P%d", max + 1);
}

void save_patient_record(const char *path, const Case *c, const char *record_type,
                              const Patient *p, const char *report_date,
                              const char *report_title, const char *report_notes,
                              const char *appt_date, const char *appt_time,
                              const char *appt_doctor, const char *appt_status,
                              const char *doctor_category, const char *appt_reason) {
    FILE *f = fopen(path, "a");
    if (!f) { printf("Cannot append to %s.\n", path); return; }
    fprintf(f, "%s,%s,%d,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f",
            c->case_id, c->is_synthetic, c->age, c->body_temperature, c->vital_signs[0], c->vital_signs[1],
            c->vital_signs[2], c->vital_signs[3], c->vital_signs[4]);
    for (int i = 0; i < SYMPTOM_COUNT; ++i) fprintf(f, ",%d", c->symptom_present[i]);
    fprintf(f, ",%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\n",
            c->case_category, c->urgency_label, c->recommended_specialty,
            record_type ? record_type : "", p ? p->patient_id : "", p ? p->pin : "",
            p ? p->name : "", p ? p->sex : "", p ? p->phone : "",
            report_date ? report_date : "", report_title ? report_title : "",
            report_notes ? report_notes : "", appt_date ? appt_date : "",
            appt_time ? appt_time : "", appt_doctor ? appt_doctor : "",
            appt_status ? appt_status : "", doctor_category ? doctor_category : "",
            appt_reason ? appt_reason : "");
    fclose(f);
}

void save_training_case(const Case *c) {
    FILE *f = fopen(BOOK_FILE, "a");
    if (!f) { printf("Cannot append to %s.\n", BOOK_FILE); return; }
    fprintf(f, "%s,%s,%d,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f",
            c->case_id, c->is_synthetic, c->age, c->body_temperature, c->vital_signs[0], c->vital_signs[1],
            c->vital_signs[2], c->vital_signs[3], c->vital_signs[4]);
    for (int i = 0; i < SYMPTOM_COUNT; ++i) fprintf(f, ",%d", c->symptom_present[i]);
    fprintf(f, ",%s,%s,%s,\n", c->case_category, c->urgency_label, c->recommended_specialty);
    fclose(f);
}

int search_patient_by_id(const char *id, Patient *p) {
    FILE *f = fopen(PATIENT_FILE, "r"); char line[CSV_LINE_CAPACITY]; int found = 0;
    if (!f) return 0;
    (void)fgets(line, sizeof(line), f);
    while (fgets(line, sizeof(line), f)) {
        char *v[CSV_FIELD_CAPACITY] = {0};
        int n = split_csv_row(line, v, CSV_FIELD_CAPACITY);
        if (n >= 29 && strcmp(v[23], "PROFILE") == 0 && strcmp(v[24], id) == 0) {
            load_case_from_csv_fields(v, &p->data);
            snprintf(p->patient_id, sizeof(p->patient_id), "%s", v[24]);
            snprintf(p->pin, sizeof(p->pin), "%s", v[25]);
            snprintf(p->name, sizeof(p->name), "%s", v[26]);
            snprintf(p->sex, sizeof(p->sex), "%s", v[27]);
            snprintf(p->phone, sizeof(p->phone), "%s", v[28]);
            found = 1; break;
        }
    }
    fclose(f); return found;
}

int search_doctor_by_id(const char *id, Doctor *doctor) {
    FILE *f = fopen(DOCTOR_FILE, "r"); char line[CSV_LINE_CAPACITY];
    if (!f) return 0;
    (void)fgets(line, sizeof(line), f);
    while (fgets(line, sizeof(line), f)) {
        char *v[12] = {0};
        if (split_csv_row(line, v, 12) >= 8 && !strcmp(v[0], id)) {
            snprintf(doctor->id, sizeof(doctor->id), "%s", v[0]);
            snprintf(doctor->name, sizeof(doctor->name), "%s", v[1]);
            snprintf(doctor->department, sizeof(doctor->department), "%s", v[2]);
            snprintf(doctor->recommended_specialty, sizeof(doctor->recommended_specialty), "%s", v[3]);
            snprintf(doctor->availability_status, sizeof(doctor->availability_status), "%s", v[4]);
            doctor->years_experience = atoi(v[5]); doctor->consultation_fee = strtod(v[6], NULL); doctor->available_slots = atoi(v[7]);
            fclose(f); return 1;
        }
    }
    fclose(f); return 0;
}

void display_doctor_list(void) {
    FILE *f = fopen(DOCTOR_FILE, "r"); char line[CSV_LINE_CAPACITY];
    if (!f) { printf("Cannot open doctor_details.csv.\n"); return; }
    printf("\nDoctor ID | Name | Specialty | Availability | Slots\n");
    (void)fgets(line, sizeof(line), f);
    while (fgets(line, sizeof(line), f)) {
        char *v[12] = {0};
        if (split_csv_row(line, v, 12) >= 8)
            printf("%s | %s | %s | %s | %s\n", v[0], v[1], v[3], v[4], v[7]);
    }
    fclose(f);
}

int is_registered_doctor(const char *id) { Doctor d; return search_doctor_by_id(id, &d); }

