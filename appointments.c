#include "medisense.h"

void display_appointments(const char *patient_id, int all) {
    FILE *f = fopen(PATIENT_FILE, "r"); char line[CSV_LINE_CAPACITY];
    AppointmentQueue queue = {{0}, 0, 0, 0};
    if (!f) { printf("No appointment records.\n"); return; }
    (void)fgets(line, sizeof(line), f);
    while (fgets(line, sizeof(line), f)) {
        char *v[CSV_FIELD_CAPACITY] = {0};
        if (split_csv_row(line, v, CSV_FIELD_CAPACITY) >= 36 && !strcmp(v[23], "APPOINTMENT") &&
            (all || !strcmp(v[24], patient_id)) && queue.item_count < MAX_QUEUED_APPOINTMENTS) {
            Appointment *a = &queue.appointments[queue.rear_index];
            snprintf(a->patient_id, sizeof(a->patient_id), "%s", v[24]);
            snprintf(a->patient_name, sizeof(a->patient_name), "%s", v[26]);
            snprintf(a->date, sizeof(a->date), "%s", v[32]);
            snprintf(a->time, sizeof(a->time), "%s", v[33]);
            snprintf(a->doctor_id, sizeof(a->doctor_id), "%s", v[34]);
            snprintf(a->status, sizeof(a->status), "%s", v[35]);
            queue.rear_index = (queue.rear_index + 1) % MAX_QUEUED_APPOINTMENTS;
            ++queue.item_count;
        }
    }
    fclose(f);
    if (!queue.item_count) { printf("No appointment requests found.\n"); return; }
    printf("Appointment requests (first-in, first-out):\n");
    while (queue.item_count) {
        Appointment *a = &queue.appointments[queue.front_index];
        printf("%s | %s | %s %s | %s | status: %s\n",
               a->patient_id, a->patient_name, a->date, a->time, a->doctor_id, a->status);
        queue.front_index = (queue.front_index + 1) % MAX_QUEUED_APPOINTMENTS;
        --queue.item_count;
    }
}

void request_appointment(const Patient *p) {
    char doctor_id[32], date[32], time[32], reason[TEXT_CAPACITY]; Doctor d;
    display_doctor_list();
    read_user_input("Select doctor ID: ", doctor_id, sizeof(doctor_id));
    if (!search_doctor_by_id(doctor_id, &d)) { printf("Doctor not found.\n"); return; }
    if (strcmp(d.availability_status, "Available") != 0 || d.available_slots <= 0) {
        printf("That doctor is not currently available for booking.\n"); return;
    }
    read_user_input("Appointment date (YYYY-MM-DD): ", date, sizeof(date));
    read_user_input("Appointment time: ", time, sizeof(time));
    read_user_input("Reason for appointment: ", reason, sizeof(reason));
    save_patient_record(PATIENT_FILE, &p->data, "APPOINTMENT", p, "", "", "", date, time,
                      d.id, "PENDING", "", reason);
    printf("Appointment request saved as PENDING for %s (%s).\n", d.name, d.recommended_specialty);
}
